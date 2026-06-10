# FMIT — Codebase Assessment

_Exploratory review, 2026-06-10. No code changed. Findings + prioritized backlog._

## Summary

FMIT is a mature, cleanly-architected C++/Qt real-time instrument tuner, ~14.5K LOC,
originally authored by Gilles Degottex in **2004**. The DSP and the layered architecture
are genuinely good. The main liabilities are all age-related: it's effectively C++98,
there are **no tests**, and the Qt5→Qt6 migration is unfinished (the default Windows
audio backend won't build on Qt6).

## Strengths

- **Clean layering**: UI (`CustomInstrumentTunerForm`) → `CaptureThread` → DSP (`libs/Music/`)
  → platform backends. Qt signals/slots across boundaries.
- **Pluggable audio backends**: `CaptureThreadImpl{Qt,ALSA,JACK,PortAudio,OSS}` all implement
  one interface, selected at compile time via `CONFIG` flags. Textbook strategy pattern.
- **Reusable LGPL libraries** (`libs/Music`, `libs/CppAddons`) cleanly separated from the GPL app.
- **Mature project**: 11 translations (Weblate), Debian + Windows (Inno Setup) packaging, CI.

## Weaknesses (evidence-backed)

### 1. No automated tests — highest-value gap
Correctness rests entirely on "does it compile." DSP code is the *easiest* thing to test:
feed a synthetic 440 Hz sine into `CombedFT`/`FreqAnalysis`, assert detected f0. There is
currently zero test infrastructure.

### 2. Qt6 audio backend is broken, not just "needs refactoring"
`fmit.pro:22` admits acs_qt "needs solid refactoring for working with Qt6." Confirmed:
`src/CaptureThreadImplQt.cpp` uses Qt5 symbols **removed in Qt6**, with *no* version guards:
- `QAudioDeviceInfo` → now `QAudioDevice`
- `QAudioInput` → now `QAudioSource`
- `QAudioFormat::setByteOrder/setCodec/setSampleSize/setSampleType` → all gone
So "compiles on qt6" (commit 589eae4) only holds with acs_qt disabled. The **default Windows
backend cannot build on Qt6.**

### 3. Aging C++ (C++98, mandated by CLAUDE.md)
- `using namespace std;` in **headers** (`CaptureThread.h:25` and `GLErrorHistory.h`,
  `GLFreqStruct.h`, etc.) — leaks into every including TU.
- Thread flags are `volatile bool` (13 uses) instead of `std::atomic` — not a real
  memory-ordering guarantee.
- 257 raw `new`/`delete` sites; function-pointer dispatch (`decodeValue`, `addValue`) and
  `friend`-based state sharing instead of modern alternatives.
- Only **2** occurrences of modern C++ (`nullptr`/`auto`/`override`/smart ptrs) in the whole tree.

### 4. Scattered tech debt
- ~30 `TODO`/`FIXME` markers, several flagging unresolved threading concerns
  (e.g. "need to keep alsa thread alive to let PortAudio work after ALSA").
- Dead commented-out code in every capture backend.
- `CustomInstrumentTunerForm.cpp` is a 1253-line god-object orchestrating everything.

### 5. Build system
qmake only. qmake is in long-term maintenance mode; Qt's own future is CMake. AppVeyor still
pins **Qt 5.10** (2018).

## Prioritized backlog

| # | Item | Effort | Value | Risk |
|---|------|--------|-------|------|
| 1 | DSP test harness (synthetic signal → assert f0) | Low | **High** | Low — additive |
| 2 | `using namespace std` out of headers; `volatile`→`std::atomic` | Low | Med | Low |
| 3 | Finish Qt6 audio backend (`QAudioSource`/`QAudioDevice`) w/ version guards | Med | **High** | Med |
| 4 | CMake build alongside qmake | Med | Med | Low |
| 5 | Refresh CI (current Qt5 + a Qt6 matrix leg) | Low | Med | Low |
| 6 | Incremental C++17 modernization | High | Med | Med |
| 7 | Decompose `CustomInstrumentTunerForm` god-object | High | Med | Med — wide blast radius |

**Recommended first move:** #1 (tests) then #3 (Qt6), because every other change is safer once
a DSP safety net exists, and #3 is the one user-facing thing that's actually broken on modern Qt.
