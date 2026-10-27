# TEIL2 REVOLO (VST3 Plugin para FL Studio)

Réplica 1:1 fotorrealista y emulación fiel de hardware del pedal de efectos **TEIL2 REVOLO** de *TEILE Elektronik (Berlín)*, diseñado para DJs y producción en FL Studio (Reverb + Tremolo + White Noise + Low-Cut Filter + Tap Tempo).

---

## 🎛️ Controles y Funciones

### Fila Superior (Reverb Engine)
* **REVERB:** Nivel de volumen / envío de la señal reverberada (0% a 100%).
* **CHARACTER:** Ajusta el espaciado y difusión de las reflexiones tempranas (*early reflections*), expandiendo la dimensión estéreo.
* **LENGTH:** Longitud del decaimiento (*decay time*) de la cola de la reverb (desde un espacio íntimo hasta un wash infinito).
* **AIR:** Brillo y presencia de altas frecuencias (*high-shelf shimmer*) en la cola de la reverb.

### Fila Media (Modulación y Textura)
* **NOISE:** Generador interno de ruido blanco inyectado directamente a la entrada del tanque de reverb (ideal para crear transiciones, builds y drops masivos).
* **TREMOLO:** Profundidad de la modulación de amplitud rítmica.
* **TEMPO:** Frecuencia del oscilador del trémolo (desde 0.2 Hz hasta 40 Hz). Sincronizable en vivo con el botón TAP.

### Fila Inferior
* **FILTER:** Filtro pasa-altos / corte de graves (20 Hz a 20 kHz, 24 dB/oct) sobre la señal con efecto para mantener limpia la zona baja de la mezcla y evitar saturar el kick y el bajo.

### Conmutadores
* **MIX / WET (Switch Horizontal):**
  * `MIX`: Modo inserción en canal de mezcla (mezcla de señal limpia + efecto).
  * `WET`: 100% de señal procesada sin paso de señal seca (modo ideal para canales auxiliares / pistas de envío *Send & Return* en FL Studio).
* **MOM / ONF (Switch Vertical):**
  * `MOM` (Momentary): El efecto solo se activa mientras mantienes presionado el botón grande izquierdo. Al soltarlo, se corta la entrada pero la cola de reverb sigue sonando naturalmente (*trails/spillover*).
  * `ONF` (Toggle/Latching): Cada pulsación enciende o apaga el efecto de forma sostenida.

### Pulsadores Gigantes Arcade y LEDs
* **Pulsador Izquierdo (Engage / Momentary Footswitch):**
  * Disparador principal del efecto con animación física.
  * **LED Rojo (ONF):** Brilla intensamente en rojo neón cuando el efecto está activo; se atenúa al desactivarse.
* **Pulsador Derecho (TAP Footswitch):**
  * Botón de Tap Tempo. Toca el botón 2 o más veces al ritmo deseado para fijar la velocidad del trémolo automáticamente.
  * **LED Verde (TAP):** Pulsa rítmicamente en tiempo real al compás de la velocidad actual del trémolo.

---

## 🚀 Cómo Compilar el Plugin (Automático con GitHub Actions)

No necesitas instalar Visual Studio ni compiladores pesados en tu computadora:

1. Sube los archivos de esta carpeta a tu repositorio en **GitHub**.
2. Ve a la pestaña **Actions** en tu repositorio de GitHub.
3. El flujo **"Build plugin"** se iniciará automáticamente (o haz clic en *Run workflow*). En ~3 a 4 minutos terminará.
4. En los resultados de la compilación, descarga el archivo en **Artifacts**:
   * `Teil2-Revolo-windows-latest.zip` (para Windows)
5. Al descomprimir el zip, obtendrás la carpeta `Teil2 Revolo.vst3`.
6. Cópiala directamente a la carpeta estándar de plugins VST3 de Windows:
   `C:\Program Files\Common Files\VST3\`
7. Abre **FL Studio** → Menú **Options** → **Manage plugins** → Haz clic en **Find installed plugins**.
8. ¡Listo! Busca `Teil2 Revolo` y agrégalo a tu mezclador de FL Studio.

---

## 💻 Compilación Local en Windows (Opcional)

Si tienes instalado Visual Studio 2022 con herramientas C++, CMake y Git:
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target AndYouRevTrem_VST3 --parallel
```
El archivo compilado se generará en `build/AndYouRevTrem_artefacts/Release/VST3/Teil2 Revolo.vst3`.
