# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**FMIT (Free Music Instrument Tuner)** — a cross-platform Qt desktop application for real-time musical instrument tuning. It captures audio, detects the fundamental frequency (f0) using DSP algorithms, and visualizes the result via multiple OpenGL views.

Current version: 1.4.6 (per `README.txt` and `git describe --tags`, which is also how `fmit.pro` derives `FMITVERSION`; verified after the upstream v1.4.6 merge, commit `eaf6b6f`). Source: https://github.com/gillesdegottex/fmit

## Build System

Uses **qmake** (Qt build system). No CMake, no npm, no Makefile written manually.

**The project is Qt6-only as of the upstream v1.4.6 merge.** Do not reintroduce Qt5 guards, `qmake-qt5`/`qtX-qt5` invocations, or Qt5 package names.

### Linux/macOS

Never built locally in this fork; the recipe below is what upstream CI (`.github/workflows/build.yml`, `ubuntu-24.04` and `macos-latest` jobs) runs — treat it as attributed CI behavior, not a verified local command.

```bash
# Linux (ubuntu-24.04, per .github/workflows/build.yml)
qmake6 "CONFIG+=acs_alsa" fmit.pro
make -j$(nproc)
make lrelease   # compile translation .ts → .qm files

# macOS (macos-latest, Homebrew qt, per .github/workflows/build.yml)
qmake "FFT_LIBDIR=$FFT_PATH" fmit.pro   # FFT_PATH = brew --prefix fftw
make -j$(sysctl -n hw.ncpu)
make lrelease
```

### Windows

Verified locally: MSVC 2022 Build Tools + Qt 6.8.3 (`msvc2022_64`) + FFTW3 via vcpkg, built out-of-source with `nmake` (not `jom` — see note below).

```bat
call "F:\VS\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set PATH=F:\Qt\6.8.3\msvc2022_64\bin;%PATH%
cd /d F:\FMIT-Error\fmit\build-qt6
qmake "FFT_LIBDIR=F:/vcpkg/installed/x64-windows" ..\fmit.pro
nmake -f Makefile.Release
```

`jom` does **not** work in this environment: on the Makefiles that Qt 6.8.3's qmake generates here, `jom.exe -f Makefile.Release` fails with a false "dependent does not exist" error. Every successful Windows build in this fork used `nmake -f Makefile.Release` instead. (Upstream CI's Windows job still uses `jom` successfully in its own environment — see `.github/workflows/build.yml` — so this is an environment-specific quirk, not a universal rule.)

### Audio capture backends (CONFIG flags)

| Flag | Backend | Platform |
|------|---------|----------|
| `acs_qt` | Qt Multimedia | All platforms (unconditionally enabled by default in `fmit.pro`) |
| `acs_alsa` | ALSA | Linux only |
| `acs_jack` | JACK | Linux only |
| `acs_portaudio` | PortAudio | Cross-platform |
| `acs_oss` | OSS | Linux only |

### Dependencies

- Qt 6.x (Linux, per CI): `qt6-base-dev`, `qt6-declarative-dev`, `qt6-tools-dev`, `qt6-multimedia-dev`, `qt6-svg-dev`
- Qt 6.x (Windows, verified locally): Qt 6.8.3 `msvc2022_64` kit (installs qmake + lrelease under `\bin`)
- Qt 6.x (macOS, per CI): Homebrew `qt`
- FFTW3: `libfftw3-dev` (Linux, per CI); vcpkg `fftw3:x64-windows` (Windows, verified locally); Homebrew `fftw` (macOS, per CI)
- Optional (Linux, per CI): `libasound2-dev`, `libpulse-dev`

Full instructions in `INSTALL.txt`.

## Testing and Linting

A standalone DSP test harness lives in `tests/` (`tests/dsp_test.pro` + `tests/dsp_test.cpp`). It feeds synthetic signals into the `CombedFT` pitch detector and asserts the detected f0 — accuracy (pure tone, harmonics, noise) and frame-to-frame stability — independent of the GUI and audio hardware. It builds and runs separately from the app; see `tests/README.md`. The harness returns a non-zero exit code on failure, so it is CI-friendly.

Verified locally on Windows (MSVC/Qt6, out-of-source in `build-tests`, `nmake -f Makefile.Release`, `fftw3.dll` copied next to the exe): 15/15 checks passed, exit code 0.

There is no linter configured. CI is GitHub Actions (`.github/workflows/build.yml`), building on a three-platform matrix (`ubuntu-24.04`, `macos-latest`, `windows-2025`) and validating that the main build compiles cleanly, so also watch compiler warnings during the build.

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
