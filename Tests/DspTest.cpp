// Prueba offline del DSP: sin host, sin interfaz. Verifica que no haya NaN, que la cola decaiga,
// que Length/Filter/Tremolo/Noise/ON-OFF se comporten como se espera.
#include "PluginProcessor.h"
#include <cstdio>

static int failures = 0;
#define CHECK(cond, msg) do { bool ok_ = (cond); std::printf("[%s] %s\n", ok_ ? " OK " : "FAIL", msg); if (!ok_) ++failures; } while (0)

struct Result { float peak = 0, rmsTail = 0, rmsLate = 0; bool bad = false; };

static void setParam (RevTremProcessor& p, const char* id, float realValue)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

static void settle (RevTremProcessor& p, double sr) { p.prepareToPlay (sr, 512); }

static float rms (const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b && i < v.size(); ++i) s += (double) v[i] * v[i];
    return (float) std::sqrt (s / (double) std::max<size_t> (1, b - a));
}

// Impulso en t=0, procesa `seconds` segundos. Devuelve salida L.
static std::vector<float> renderImpulse (RevTremProcessor& p, double sr, double seconds, float impulse = 1.f)
{
    const int block = 512;
    const size_t total = (size_t) (seconds * sr);
    std::vector<float> out; out.reserve (total);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    bool first = true;
    while (out.size() < total)
    {
        buf.clear();
        if (first) { buf.setSample (0, 0, impulse); buf.setSample (1, 0, impulse); first = false; }
        p.processBlock (buf, midi);
        for (int i = 0; i < block; ++i) out.push_back (buf.getSample (0, i));
    }
    return out;
}

static bool hasBad (const std::vector<float>& v)
{
    for (float x : v) if (! std::isfinite (x) || std::abs (x) > 10.f) return true;
    return false;
}

int main()
{
    const double sr = 48000.0;
    auto make = [&] { auto p = std::make_unique<RevTremProcessor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };

    // 1) Reverb por defecto: hay cola, es finita y decae
    {
        auto p = make();
        setParam (*p, "reverb", 1.f);
        auto o = renderImpulse (*p, sr, 10.0);
        float early = rms (o, 0, (size_t) (0.5 * sr)), late = rms (o, (size_t) (8 * sr), (size_t) (10 * sr));
        std::printf("   default: rms 0-0.5s=%.5f  rms 8-10s=%.7f\n", early, late);
        CHECK (! hasBad (o), "sin NaN/Inf ni explosiones (default)");
        CHECK (early > 1e-4f, "el reverb produce cola");
        CHECK (late < early * 0.1f, "la cola decae con el tiempo");
    }

    // 2) Length: más largo = cola más duradera
    {
        auto a = make(); setParam (*a, "reverb", 1.f); setParam (*a, "length", 0.1f);
        auto b = make(); setParam (*b, "reverb", 1.f); setParam (*b, "length", 0.95f);
        auto oa = renderImpulse (*a, sr, 6.0), ob = renderImpulse (*b, sr, 6.0);
        float la = rms (oa, (size_t) (3 * sr), (size_t) (5 * sr)), lb = rms (ob, (size_t) (3 * sr), (size_t) (5 * sr));
        std::printf("   cola 3-5s: length 0.1 -> %.7f   length 0.95 -> %.7f\n", la, lb);
        CHECK (lb > la * 5.f, "Length largo => cola mucho más larga");
        CHECK (! hasBad (ob), "Length al máximo sigue estable");
    }

    // 3) Filtro: low-cut alto atenúa un seno grave (100 Hz) y deja pasar uno agudo (8 kHz)
    {
        auto run = [&] (float cutoff, float freq)
        {
            auto p = make(); setParam (*p, "reverb", 1.f); setParam (*p, "filter", cutoff); setParam (*p, "wetmode", 1.f);
            setParam (*p, "length", 0.5f); settle (*p, sr);
            juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; std::vector<float> o; double ph = 0;
            for (int b = 0; b < 400; ++b)
            {
                for (int i = 0; i < 512; ++i) { float x = 0.5f * (float) std::sin (ph); ph += 2.0 * 3.14159265358979 * freq / sr; buf.setSample (0, i, x); buf.setSample (1, i, x); }
                p->processBlock (buf, m);
                for (int i = 0; i < 512; ++i) o.push_back (buf.getSample (0, i));
            }
            return rms (o, o.size() / 2, o.size());
        };
        float lowOpen = run (20.f, 100.f), lowCut = run (2000.f, 100.f), hiOpen = run (20.f, 8000.f), hiCut = run (2000.f, 8000.f);
        std::printf("   100 Hz: filtro 20Hz=%.5f  filtro 2kHz=%.5f | 8 kHz: 20Hz=%.5f  2kHz=%.5f\n", lowOpen, lowCut, hiOpen, hiCut);
        CHECK (lowCut < lowOpen * 0.05f, "Filter 2 kHz atenúa >26 dB un seno de 100 Hz");
        CHECK (hiCut > hiOpen * 0.7f, "Filter 2 kHz deja pasar los agudos (8 kHz)");
    }

    // 4) Tremolo: la envolvente de la cola oscila a la velocidad de Tempo
    {
        auto p = make(); setParam (*p, "reverb", 1.f); setParam (*p, "length", 0.9f);
        setParam (*p, "tremolo", 1.f); setParam (*p, "tempo", 4.f);
        auto o = renderImpulse (*p, sr, 3.0);
        // energía por ventana de 10 ms entre 0.5 y 2.5 s; contamos cruces de la media
        std::vector<float> env; for (size_t i = (size_t) (0.5 * sr); i + 480 < (size_t) (2.5 * sr); i += 480) env.push_back (rms (o, i, i + 480));
        double mean = 0; for (float e : env) mean += e; mean /= (double) env.size();
        int crossings = 0; for (size_t i = 1; i < env.size(); ++i) if ((env[i-1] - mean) * (env[i] - mean) < 0) ++crossings;
        float mn = *std::min_element (env.begin(), env.end()), mx = *std::max_element (env.begin(), env.end());
        std::printf("   tremolo 4 Hz sobre 2 s: cruces=%d (esperado ~16)  min/max env=%.5f/%.5f\n", crossings, mn, mx);
        CHECK (crossings >= 12 && crossings <= 20, "Tremolo oscila ~ a la frecuencia de Tempo (4 Hz)");
        CHECK (mn < mx * 0.3f, "Profundidad de tremolo audible");
    }

    // 5) Noise: sin entrada, el ruido alimenta el reverb
    {
        auto a = make(); setParam (*a, "reverb", 1.f); setParam (*a, "noise", 0.f);
        auto b = make(); setParam (*b, "reverb", 1.f); setParam (*b, "noise", 0.8f);
        auto oa = renderImpulse (*a, sr, 2.0, 0.f), ob = renderImpulse (*b, sr, 2.0, 0.f);
        float ra = rms (oa, (size_t) (0.5 * sr), (size_t) (1.5 * sr)), rb = rms (ob, (size_t) (0.5 * sr), (size_t) (1.5 * sr));
        std::printf("   sin entrada: noise 0 -> %.7f   noise 0.8 -> %.5f\n", ra, rb);
        CHECK (ra < 1e-6f, "Noise=0 y sin entrada => silencio");
        CHECK (rb > 1e-3f, "Noise>0 genera ruido a través del reverb");
    }

    // 6) ON/OFF: al apagar, la cola sigue sonando pero no entra señal nueva
    {
        auto p = make(); setParam (*p, "reverb", 1.f); setParam (*p, "length", 0.8f);
        setParam (*p, "on", 0.f); settle (*p, sr);
        auto o = renderImpulse (*p, sr, 2.0);
        float r = rms (o, (size_t) (0.2 * sr), (size_t) (1.5 * sr));
        std::printf("   OFF: rms wet=%.8f (dry pasa en la muestra 0)\n", r);
        CHECK (r < 1e-5f, "ON=off => nada entra al reverb");
    }
    {
        auto p = make(); setParam (*p, "reverb", 1.f); setParam (*p, "length", 0.8f);
        const int block = 512; juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        buf.clear(); buf.setSample (0, 0, 1.f); buf.setSample (1, 0, 1.f);
        p->processBlock (buf, m);                      // entra impulso con ON
        setParam (*p, "on", 0.f);                      // apagar: la cola debe seguir
        std::vector<float> o;
        for (int b = 0; b < 200; ++b) { buf.clear(); p->processBlock (buf, m); for (int i = 0; i < block; ++i) o.push_back (buf.getSample (0, i)); }
        float r = rms (o, 0, (size_t) (1.0 * sr));
        std::printf("   apagar tras golpe: rms cola=%.5f\n", r);
        CHECK (r > 1e-4f, "La cola sigue sonando al soltar ON/OFF (efecto 'dip & release')");
    }

    // 7) Mix vs Wet: en Wet no pasa seco
    {
        auto a = make(); setParam (*a, "reverb", 0.f);   setParam (*a, "wetmode", 0.f); settle (*a, sr);
        auto b = make(); setParam (*b, "reverb", 0.f);   setParam (*b, "wetmode", 1.f); settle (*b, sr);
        auto oa = renderImpulse (*a, sr, 0.1), ob = renderImpulse (*b, sr, 0.1);
        std::printf("   muestra 0: Mix=%.3f  Wet=%.3f\n", oa[0], ob[0]);
        CHECK (std::abs (oa[0] - 1.f) < 1e-3f, "Mix: el seco pasa intacto");
        CHECK (std::abs (ob[0]) < 1e-3f, "Wet: el seco no pasa");
    }

    // 7b) El wet no debe tener componente directo en t=0 (solo reflexiones/cola)
    {
        auto p = make(); setParam (*p, "reverb", 1.f); setParam (*p, "wetmode", 1.f); settle (*p, sr);
        auto o = renderImpulse (*p, sr, 0.2);
        float first5ms = 0; for (size_t i = 0; i < (size_t) (0.005 * sr); ++i) first5ms = std::max (first5ms, std::abs (o[i]));
        std::printf("   wet: pico en los primeros 5 ms = %.5f (o[0]=%.5f)\n", first5ms, o[0]);
        CHECK (first5ms < 0.05f, "Wet sin fuga directa del impulso");
    }

    // 8) Estrés: todo al máximo, ruido blanco fuerte de entrada
    {
        auto p = make();
        for (auto id : { "reverb", "character", "length", "air", "noise", "tremolo" }) setParam (*p, id, 1.f);
        setParam (*p, "tempo", 40.f); setParam (*p, "filter", 20000.f);
        juce::Random r; juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; bool bad = false; float pk = 0;
        for (int b = 0; b < 800; ++b)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, r.nextFloat() * 2.f - 1.f);
            p->processBlock (buf, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { float x = buf.getSample (c, i); if (! std::isfinite (x)) bad = true; pk = std::max (pk, std::abs (x)); }
        }
        std::printf("   estrés: pico=%.3f\n", pk);
        CHECK (! bad && pk < 8.f, "Estrés al máximo: sin NaN y sin explotar");
    }

    std::printf("\n%s (%d fallos)\n", failures == 0 ? "TODO OK" : "HAY FALLOS", failures);
    return failures == 0 ? 0 : 1;
}
