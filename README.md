# BLESSED SCRATCH — Vocal FX (VST3)

Plugin de efectos vocales para producción musical: **scratch vocal generativo**, **stop tape** y **combo FX**.
Windows (VST3 + Standalone) y macOS (VST3 + Standalone, universal Apple Silicon/Intel). Hecho con JUCE 8 (C++17).

## Cómo se usa
1. Pon el plugin en una pista de voz. Pulsa **REC**, reproduce la voz y vuelve a pulsar REC (o **LOAD** / arrastra un audio).
2. Arrastra la selección en el waveform.
3. Elige la sección (**SCRATCH / STOP TAPE / COMBO FX**), ajusta los controles y pulsa **GENERATE**
   (**REGENERATE** = nueva variación). **PREVIEW** reproduce el patrón (sincronizado al host si HOST está activo).
4. **EXPORT WAV** renderiza el patrón a un archivo. **A/B** compara dos ajustes.

## Compilar en GitHub (automático)
Cada `push` a `main` ejecuta `.github/workflows/build.yml` y compila Windows y macOS.
Descarga los `.zip` en **Actions → último run → Artifacts**. Si subes un tag `v1.0.0` se crea un **Release** con los zips.

## Instalar
- Windows: copia `BLESSED SCRATCH.vst3` a `C:\Program Files\Common Files\VST3\`
- macOS: copia a `/Library/Audio/Plug-Ins/VST3/` (o `~/Library/Audio/Plug-Ins/VST3/`).
  Al no estar firmado con Apple Developer ID, si macOS lo bloquea: `xattr -cr "BLESSED SCRATCH.vst3"`.

## Compilar local
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target BlessedScratch_VST3
```
Requiere CMake 3.22+ y un compilador C++17 (MSVC 2022 / Xcode). JUCE se descarga solo.

## Licencias
Código del plugin © tuyo. JUCE: licencia AGPLv3/comercial (https://juce.com/juce-licensing). Fuentes Anton y Barlow Condensed: SIL OFL (ver `Assets/fonts`).
