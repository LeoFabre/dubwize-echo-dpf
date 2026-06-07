#include <cmath>
#include <cstdio>
#include <cstdlib>
#define EXPECT_NEAR(a, b, eps) do {                                          \
    const double _a = double(a), _b = double(b), _e = double(eps);           \
    if (std::abs(_a - _b) > _e) {                                            \
        std::fprintf(stderr, "FAIL %s:%d: %.9f != %.9f (eps=%.3g)\n",        \
            __FILE__, __LINE__, _a, _b, _e); std::exit(1);                   \
    } } while (0)
#define EXPECT_TRUE(c) do { if (!(c)) {                                      \
    std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);        \
    std::exit(1); } } while (0)
#include <algorithm>
#include "Lfo.hpp"
using namespace dubwize;
int main() {
    const float fs = 48000.0f;
    FastMathLfo lfo; lfo.reset(fs);
    lfo.setParams(1.0f, 1.0f, FastMathLfo::LFOWave::Tri, FastMathLfo::LFOPolarity::Unipolar);
    float mn = 1e9f, mx = -1e9f;
    for (int i = 0; i < (int) fs; ++i) { float v = lfo.getNextSample(0.0f); mn = std::min(mn,v); mx = std::max(mx,v); }
    EXPECT_TRUE(mn >= -1e-4f && mx <= 1.0f + 1e-4f);
    EXPECT_TRUE(mx > 0.9f && mn < 0.1f);

    FastMathLfo s; s.reset(fs);
    s.setParams(2.0f, 1.0f, FastMathLfo::LFOWave::Sin, FastMathLfo::LFOPolarity::Bipolar);
    double sum = 0; for (int i = 0; i < (int) fs; ++i) sum += s.getNextSample(0.0f);
    EXPECT_NEAR(sum / fs, 0.0, 1e-2);

    FastMathLfo z; z.reset(fs); z.setParams(0.0f, 1.0f, FastMathLfo::LFOWave::Sin, FastMathLfo::LFOPolarity::Bipolar);
    EXPECT_NEAR(z.getNextSample(0.0f), 0.0f, 1e-9);
    std::printf("test_lfo OK\n"); return 0;
}
