<p align="center">
  <img src="docs/icon-192.png" width="100">
</p>

# SPEKTRE VST3

The [Spektre](https://github.com/TheoGal/spektre) spectrum analyzer as a plugin for Windows and macOS (VST3, plus AU on macOS), built with JUCE. Put it on any track or bus in your DAW: the audio passes through untouched and Spektre shows its spectrum.

![Spektre VST3](docs/screenshot.png)
<!-- placeholder: add docs/screenshot.png -->

### disclaimer
Im into web developing the last 10 years but this app was 100% vibe-coded with Claude - I simply inspected the code afterwards.

## Features

- **Calibrated dBFS:** a 0 dBFS sine reads 0 dB, the same math as the web version.
- **Precise peak readout:** interpolated peak frequency and level, with the note name and cents.
- **Display modes:** line, or 1/3, 1/6 and 1/12-octave bars, with optional peak hold (decay or max).
- **Live stats:** RMS and sample-peak meters.
- **Resolution control:** FFT size from 2048 to 32768, with the bin resolution shown live.
- **Analysis options:** dB/oct slope, channel select (L+R, Left or Right), smoothing, display range and freeze.
- **Spectrogram:** an optional scrolling view under the main plot.
- **Cursor readout:** hover the plot for frequency, level and note.
- **Remembers its view:** settings are saved with your project, per plugin instance.
- **Resizable window.**

### dB/oct slope setting

Music naturally carries far more energy in the low end, so a flat (unsloped) spectrum of a well-balanced mix falls steeply from left to right. That makes tonal balance hard to judge.

Tilting the display by 4.5 dB per octave (the default) makes a balanced mix look roughly flat. It's the same convention as the default slope in Voxengo SPAN and other mixing analyzers.

The slope changes only the plot. The peak level and cursor readouts always show the true, unsloped dBFS.

## Install

Download from the [latest release](https://github.com/TheoGal/spektre-vst/releases/latest).

**Windows** (`Spektre-Windows-x64.zip`)

1. Copy the `Spektre.vst3` folder to `C:\Program Files\Common Files\VST3\`.
2. Rescan plugins in your DAW. Spektre shows up as an effect (category: Analyzer).

**macOS** (`Spektre-macOS-universal.zip`, Apple Silicon and Intel)

1. Copy `Spektre.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and, for Logic or GarageBand, `Spektre.component` (Audio Unit) to `~/Library/Audio/Plug-Ins/Components/`.
2. The builds are not notarized by Apple, so macOS blocks them at first. Run this once in Terminal:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Spektre.vst3
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Spektre.component
   ```
3. Restart your DAW.

Both zips also contain a standalone app (`Spektre.exe` / `Spektre.app`) that analyzes your audio input device. It has no audio output, so it can't cause feedback.

## Build it yourself

You need CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows). JUCE is downloaded automatically.

```
cmake -S . -B build
cmake --build build --config Release --target Spektre_VST3
```

The plugin ends up in `build/Spektre_artefacts/Release/VST3/` (on macOS also `AU/`).

Every push to GitHub builds the Windows and macOS versions automatically and checks them with [pluginval](https://github.com/Tracktion/pluginval) (see the Actions tab). Pushing a tag like `v1.0.0` also attaches both zips to the release.

### Tests

`cmake -S . -B build -DSPEKTRE_BUILD_TESTS=ON` adds a test program that feeds known sine waves through the plugin and checks the readings (level within 0.1 dB, frequency within 0.5 Hz), checks state save and restore, and renders the UI to PNG.

## License

AGPLv3, because Spektre uses the JUCE framework under its open-source (AGPLv3) license. In short: use, share and modify it freely, but changes you distribute must stay open source under the same license.

The bundled fonts (Barlow Condensed, JetBrains Mono) are under the SIL Open Font License, see `Resources/fonts/`.
