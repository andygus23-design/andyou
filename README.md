# AndYou RevTrem

Plugin de reverb + tremolo (VST3 / Standalone), JUCE 8.

## Controles
| Perilla | Qué hace |
|---|---|
| Reverb | Nivel del reverb |
| Character | Early reflections / tamaño del cuarto |
| Length | Duración de la cola |
| Air | Brillo (altas frecuencias) de la cola |
| Noise | Ruido blanco que alimenta al reverb |
| Tremolo | Profundidad del tremolo (después del reverb) |
| Tempo | Velocidad del tremolo, 0.2 – 40 Hz |
| Filter | Low-cut 20 Hz – 20 kHz |
| Mix/Wet | Mix = seco + reverb · Wet = solo reverb (para send/return) |
| On / Momentary | Activa la entrada al reverb; la cola sigue sonando al apagar |

## Compilar (sin instalar nada: GitHub Actions)
1. Sube esta carpeta a un repositorio nuevo en GitHub.
2. Pestaña **Actions** → "Build plugin" → espera ~10 min.
3. Descarga el artefacto `AndYouRevTrem-windows-latest` → carpeta `AndYou RevTrem.vst3`.
4. Cópiala a `C:\Program Files\Common Files\VST3\`.
5. FL Studio → Options → Manage plugins → **Find installed plugins**.

## Compilar local (Windows)
Requiere Visual Studio 2022 (C++), CMake y Git:
    cmake -B build
    cmake --build build --config Release --target AndYouRevTrem_VST3

## Prueba del DSP sin DAW
    cmake -B build -DREVTREM_BUILD_TESTS=ON
    cmake --build build --target DspTest
