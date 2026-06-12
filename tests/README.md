# FMIT tests

A small, standalone **DSP test harness** for the pitch detection algorithm.
It is intentionally GUI-free and hardware-free: it feeds synthetic signals
straight into `Music::CombedFT` (the `libs/Music/` DSP), so it needs only a
compiler, Qt Core and FFTW — no audio device, no OpenGL.

It is the safety net for the pitch-detection work (fork issue #3), meant to be
in place before changing the stability/smoothing path (issue #1 / upstream
[#137](https://github.com/gillesdegottex/fmit/issues/137)).

## What it checks

`dsp_test.cpp` runs four groups and returns a non-zero exit code if any fail:

1. **Pure-sine accuracy** — detected f0 within tolerance (cents) at 110–440 Hz.
2. **Harmonics** — fundamental must win when overtones are present.
3. **Noise** — accuracy with added white noise.
4. **Stability** — slides the analysis window across a long steady 293.66 Hz
   tone and reports the spread of f0 (bias / stddev / peak-to-peak, in cents),
   for clean vs. realistic (harmonics + noise) input. This is the objective
   metric for the issue #1 "wobble".

The tolerances and the stability guard are deliberately loose: they document
the *current* behaviour and catch regressions. Tighten them as #1 is improved.

## Build & run

The harness is a separate qmake project; it does **not** build with the app.

### Windows (MinGW)

```powershell
# from the tests/ directory, with Qt + MinGW on PATH
qmake "FFT_LIBDIR=F:\path\to\fftw-3.3.5-dll64" dsp_test.pro
mingw32-make -f Makefile.Release
# run (Qt5Core, the MinGW runtime and libfftw3-3.dll must be reachable)
release\dsp_test.exe
```

### Linux / macOS

```bash
cd tests
qmake dsp_test.pro      # qmake-qt5 on some distros
make
./dsp_test
```

`FFT_LIBDIR` mirrors `fmit.pro`: on Windows it points at the FFTW DLL folder;
on Linux the system `libfftw3` is used by default.

## Expected output

A list of `[PASS]`/`[FAIL]` lines ending in `13/13 checks passed, 0 failed`
and exit code `0`. Note that on synthetic input `CombedFT` is essentially
rock-stable (pure-sine f0 stddev ≈ 0.01 cents), which is why the #1 wobble is
suspected to live downstream (quantizer / display) or in noisier real input.
