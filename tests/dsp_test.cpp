// Part of FMIT (https://github.com/gillesdegottex/fmit).
//
// Standalone DSP test harness: feed synthetic signals into the pitch
// detection algorithm (CombedFT) and assert the detected fundamental
// frequency, plus measure its frame-to-frame stability on a steady tone.
//
// This is the safety net referenced by fork issue #3, built before touching
// the smoothing/stability work of issue #1 (upstream gillesdegottex/fmit#137).
//
// Licensed under the GNU General Public License, consistent with FMIT.

#include <Music/Music.h>
#include <Music/CombedFT.h>

#include <cstdio>
#include <cmath>
#include <deque>
#include <vector>
#include <string>

using std::deque;
using std::vector;
using std::string;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// Deterministic LCG so the "noise" tests are reproducible across runs/machines.
struct Rng
{
    unsigned long long s;
    Rng(unsigned long long seed) : s(seed) {}
    double next() // uniform in [-1, 1]
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        unsigned v = (unsigned)(s >> 33);
        return (double(v) / 2147483647.5) - 1.0;
    }
};

static double log2_(double x) { return std::log(x) / std::log(2.0); }
static double cents(double detected, double expected) { return 1200.0 * log2_(detected / expected); }

// Build one analysis window of n samples of a tone at `freq` (plus optional
// harmonics and white noise), starting at absolute sample index startN.
static deque<double> makeWindow(double freq, int sr, int n, int startN,
                                const vector<double>& harmAmps, double noiseAmp, Rng& rng)
{
    deque<double> buff;
    const double w = 2.0 * M_PI * freq / double(sr);
    for (int i = 0; i < n; ++i)
    {
        const int t = startN + i;
        double smp = 0.0;
        for (size_t h = 0; h < harmAmps.size(); ++h)
            smp += harmAmps[h] * std::sin(w * double(h + 1) * double(t));
        if (noiseAmp > 0.0)
            smp += noiseAmp * rng.next();
        buff.push_back(smp);
    }
    return buff;
}

// ---------------------------------------------------------------------------
// tiny assertion framework
// ---------------------------------------------------------------------------

static int g_fail = 0;
static int g_total = 0;

static void check(const char* name, bool ok, const string& detail)
{
    ++g_total;
    if (!ok) ++g_fail;
    std::printf("  [%s] %-10s %s\n", ok ? "PASS" : "FAIL", name, detail.c_str());
}

static string fmt(const char* f, double a, double b, double c)
{
    char buf[160];
    std::snprintf(buf, sizeof(buf), f, a, b, c);
    return string(buf);
}

// Slide the analysis window across a long steady tone and report the spread of
// the detected f0 (bias / stddev / peak-to-peak, in cents). This is the
// objective metric for the issue #1 / #137 "wobble".
static void stabilityScan(Music::CombedFT& algo, int sr, int win, double f,
                          const vector<double>& harm, double noiseAmp,
                          Rng& rng, const char* label)
{
    const int frames = 300;
    const int hop = 160; // non period-aligned, mimics the live rolling buffer
    double sumc = 0.0, sumc2 = 0.0, minc = 1e9, maxc = -1e9;
    int valid = 0;
    for (int k = 0; k < frames; ++k)
    {
        deque<double> b = makeWindow(f, sr, win, k * hop, harm, noiseAmp, rng);
        algo.apply(b);
        const double det = algo.getFondamentalFreq();
        if (det > 0)
        {
            const double c = cents(det, f);
            sumc += c; sumc2 += c * c;
            if (c < minc) minc = c;
            if (c > maxc) maxc = c;
            ++valid;
        }
    }
    const double mean = (valid > 0) ? sumc / valid : 0.0;
    double var = (valid > 0) ? (sumc2 / valid - mean * mean) : 0.0;
    if (var < 0.0) var = 0.0;
    const double sd = std::sqrt(var);
    const double ptp = (valid > 0) ? (maxc - minc) : 0.0;
    std::printf("    %-16s frames=%d valid=%d  bias=%+.2f  jitter(stddev)=%.3f  peak-to-peak=%.3f  (cents)\n",
                label, frames, valid, mean, sd, ptp);
    char sdetail[180];
    std::snprintf(sdetail, sizeof(sdetail),
                  "%s: stddev=%.3f cents (guard <50)  peak-to-peak=%.3f", label, sd, ptp);
    check("stability", valid == frames && sd < 50.0, string(sdetail));
}

// ---------------------------------------------------------------------------

int main()
{
    const int sr = 48000;
    Music::SetSamplingRate(sr);
    Music::SetAFreq(440.0);
    Music::SetSemitoneBounds(-36, 24); // ~55 Hz .. 1760 Hz, covers the test tones

    Music::CombedFT algo;
    const int win = algo.getMinSize();
    std::printf("FMIT DSP test harness\n");
    std::printf("sampling rate = %d Hz, CombedFT window = %d samples (%.1f ms), FFT bin = %.2f Hz\n",
                sr, win, 1000.0 * win / sr, double(sr) / win);

    Rng rng(123456789ULL);

    vector<double> pure(1, 1.0); // single harmonic

    // -- Test 1: pure-sine accuracy ------------------------------------------
    std::printf("\n[1] Pure sine accuracy (tolerance 20 cents):\n");
    {
        const double freqs[] = { 110.0, 146.83, 220.0, 293.66, 440.0 };
        for (int i = 0; i < 5; ++i)
        {
            const double f = freqs[i];
            deque<double> b = makeWindow(f, sr, win, 0, pure, 0.0, rng);
            algo.apply(b);
            const double det = algo.getFondamentalFreq();
            const double c = (det > 0) ? cents(det, f) : 0.0;
            check("pure", det > 0 && std::fabs(c) < 20.0,
                  fmt("f=%.2f Hz  detected=%.3f Hz  err=%+.2f cents", f, det, c));
        }
    }

    // -- Test 2: with harmonics (fundamental must win) -----------------------
    // Low frequencies are the hardest: the window is only ~2 periods of 110 Hz
    // and the FFT bin is coarse, so harmonics bias the estimate by ~25-30 cents.
    // The tolerance documents current behaviour and guards against regression;
    // the live app smooths this further downstream (MonoQuantizer).
    std::printf("\n[2] Tone + harmonics, detector must return the fundamental (tolerance 35 cents):\n");
    {
        vector<double> harm;
        harm.push_back(1.0); harm.push_back(0.6); harm.push_back(0.4); harm.push_back(0.25);
        const double freqs[] = { 110.0, 220.0, 293.66 };
        for (int i = 0; i < 3; ++i)
        {
            const double f = freqs[i];
            deque<double> b = makeWindow(f, sr, win, 0, harm, 0.0, rng);
            algo.apply(b);
            const double det = algo.getFondamentalFreq();
            const double c = (det > 0) ? cents(det, f) : 0.0;
            check("harmonics", det > 0 && std::fabs(c) < 35.0,
                  fmt("f=%.2f Hz  detected=%.3f Hz  err=%+.2f cents", f, det, c));
        }
    }

    // -- Test 3: with white noise --------------------------------------------
    std::printf("\n[3] Tone + white noise (amp 0.1, tolerance 30 cents):\n");
    {
        const double freqs[] = { 110.0, 293.66, 440.0 };
        for (int i = 0; i < 3; ++i)
        {
            const double f = freqs[i];
            deque<double> b = makeWindow(f, sr, win, 0, pure, 0.1, rng);
            algo.apply(b);
            const double det = algo.getFondamentalFreq();
            const double c = (det > 0) ? cents(det, f) : 0.0;
            check("noise", det > 0 && std::fabs(c) < 30.0,
                  fmt("f=%.2f Hz  detected=%.3f Hz  err=%+.2f cents", f, det, c));
        }
    }

    // -- Test 4: stability on a steady tone (objective measure for issue #1) --
    std::printf("\n[4] Stability on a steady 293.66 Hz tone over a sliding window\n");
    std::printf("    (quantifies fork issue #1 / upstream #137 -- clean vs realistic input):\n");
    {
        vector<double> harm;
        harm.push_back(1.0); harm.push_back(0.6); harm.push_back(0.4); harm.push_back(0.25);
        stabilityScan(algo, sr, win, 293.66, pure, 0.00, rng, "pure sine");
        stabilityScan(algo, sr, win, 293.66, harm, 0.05, rng, "harmonics+noise");
    }

    std::printf("\n%d/%d checks passed, %d failed\n", g_total - g_fail, g_total, g_fail);
    return g_fail == 0 ? 0 : 1;
}
