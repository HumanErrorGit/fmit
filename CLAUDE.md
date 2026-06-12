# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**FMIT (Free Music Instrument Tuner)** — a cross-platform Qt desktop application for real-time musical instrument tuning. It captures audio, detects the fundamental frequency (f0) using DSP algorithms, and visualizes the result via multiple OpenGL views.

Current version: 1.3.3. Source: https://github.com/gillesdegottex/fmit

## Build System

Uses **qmake** (Qt build system). No CMake, no npm, no Makefile written manually.

### Linux/macOS

```bash
mkdir build && cd build
qmake-qt5 "CONFIG+=acs_qt acs_alsa acs_jack acs_portaudio" ../fmit.pro
make
make lrelease   # compile translation .ts → .qm files
```

### Windows

```powershell
qmake "FFT_LIBDIR=C:\path\to\fftw3" fmit.pro
jom.exe -f Makefile.Release
```

### Audio capture backends (CONFIG flags)

| Flag | Backend | Platform |
|------|---------|----------|
| `acs_qt` | Qt Multimedia | All (default for Windows) |
| `acs_alsa` | ALSA | Linux only |
| `acs_jack` | JACK | Linux only |
| `acs_portaudio` | PortAudio | Cross-platform |
| `acs_oss` | OSS | Linux only |

### Dependencies

- Qt 5.x: `qtbase5-dev`, `qtmultimedia5-dev`, `libqt5opengl5-dev`, `libqt5svg5-dev`, `qttools5-dev-tools`
- FFTW3: `libfftw3-dev`
- Optional (Linux): `libasound2-dev`, `libjack-dev`, `portaudio19-dev`

Full instructions in `INSTALL.txt`.

## Testing and Linting

A standalone DSP test harness lives in `tests/` (`tests/dsp_test.pro` + `tests/dsp_test.cpp`). It feeds synthetic signals into the `CombedFT` pitch detector and asserts the detected f0 — accuracy (pure tone, harmonics, noise) and frame-to-frame stability — independent of the GUI and audio hardware. It builds and runs separately from the app; see `tests/README.md`. The harness returns a non-zero exit code on failure, so it is CI-friendly.

There is no linter configured. CI (Travis CI / AppVeyor) validates that the main build compiles cleanly, so also watch compiler warnings during `make`.

## Architecture

### Layers

```
UI Layer          CustomInstrumentTunerForm (QMainWindow)
                  ↑ Qt signals/slots
Core Layer        CaptureThread → DSP algorithms (libs/Music/)
                  ↑
Platform Layer    ALSA / JACK / PortAudio / Qt Multimedia / FFTW3 / OpenGL
```

### Key source directories

- `src/` — Main application: main window, audio capture thread, settings
- `src/modules/` — OpenGL visualization views (dial, graph, error/volume history, waveform, FFT, stats, microtonal)
- `libs/Music/` — LGPL DSP library: pitch detection algorithms, FFT wrapper, filtering, note representation
- `libs/CppAddons/` — LGPL C++ utility library: math, polynomial fitting, string helpers
- `ui/` — Qt Designer `.ui` files for main window and config dialog
- `tr/` — Qt Linguist `.ts` translation files (11 languages)
- `scales/` — Scala format `.scl` microtonal scale definitions
- `distrib/` — Packaging scripts and metadata (Debian, Windows Inno Setup)

### Data flow

```
Microphone → CaptureThread → audio buffer → pitch detection algorithm
  → detected frequency + error → OpenGL views → displayed to user
```

### Main classes

| Class | File | Role |
|-------|------|------|
| `CustomInstrumentTunerForm` | `src/CustomInstrumentTunerForm.*` | Main window, orchestrates everything |
| `CaptureThread` | `src/CaptureThread.*` | Platform-agnostic audio capture, runs in separate `QThread` |
| `CombedFT` | `libs/Music/CombedFT.*` | Primary pitch detection (combed Fourier Transform) |
| `FreqAnalysis` | `libs/Music/FreqAnalysis.*` | Frequency domain analysis |
| `Filter` | `libs/Music/Filter.*` | High-pass and FIR filtering applied before pitch detection |
| `Note` | `libs/Music/Note.*` | Musical note representation and Hz ↔ note conversion |
| `MicrotonalView` | `src/modules/MicrotonalView.*` | Microtonal scale editor (largest module, ~33KB) |
| `AutoQSettings` | `src/AutoQSettings.*` | Persistent settings via `QSettings` |

### Audio capture implementations

`CaptureThread` selects the backend at compile time via the `CONFIG` flags. Each backend lives in its own `CaptureThreadImpl*.cpp` file and implements the same interface defined in `CaptureThread.h`.

### Configuration and settings

Application settings are stored via `QSettings` (platform-native location). The config dialog (`ui/ConfigForm.ui`) covers: audio device, pitch detection algorithm, amplitude threshold, filtering, display options, and note naming convention. Settings listeners pattern propagates global changes to all views.

## Code Style

- C++98 compatible (avoid C++11 features unless already present in the file being edited)
- Member variables prefixed `m_`, static members `s_`
- Qt signal/slot pattern for inter-component communication
- Comments use `//` style only
