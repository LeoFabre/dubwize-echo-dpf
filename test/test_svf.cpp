#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#define EXPECT_NEAR(a, b, eps) do {                                          \
    const double _a = double(a), _b = double(b), _e = double(eps);           \
    if (std::abs(_a - _b) > _e) {                                            \
        std::fprintf(stderr, "FAIL %s:%d: %.9f != %.9f (eps=%.3g)\n",        \
            __FILE__, __LINE__, _a, _b, _e); std::exit(1);                   \
    } } while (0)
#define EXPECT_TRUE(c) do { if (!(c)) {                                      \
    std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);        \
    std::exit(1); } } while (0)

#include "SvfFilter.hpp"
#include "DspMath.hpp"
using namespace dubwize;

static float rmsGainStaticLp(float fc, float testHz, float fs) {
    StaticSvf f; f.reset(fs);
    f.setParameters(fc, 0.707f, false, false, 0.0f, 0.0f, 0.0f, 1.0f, false); // lpfMix=1
    double acc = 0; int N = (int) fs;
    for (int i = 0; i < N; ++i) {
        float x = std::sin(kTwoPi * testHz * i / fs);
        float y = f.processSample(x);
        if (i > N/2) acc += double(y) * y;
    }
    return std::sqrt(acc / (N/2)) * std::sqrt(2.0f);
}

int main() {
    const float fs = 48000.0f;
    EXPECT_NEAR(rmsGainStaticLp(1000.0f, 100.0f,  fs), 1.0f, 0.1f);
    float hi = rmsGainStaticLp(1000.0f, 8000.0f, fs);
    EXPECT_TRUE(gainToDb(hi) < -20.0f);

    Svf sm; sm.reset(fs);
    sm.setParameters(1000.0f, 0.707f, 0.0f, 0.0f, 0.0f, 1.0f);   // bpfMix,bsfMix,hpfMix,lpfMix
    double acc = 0; int N = (int) fs;
    for (int i = 0; i < N; ++i) { float x = std::sin(kTwoPi*100.0f*i/fs); float y = sm.processSample(x); if (i>N/2) acc += double(y)*y; }
    float g = std::sqrt(acc/(N/2)) * std::sqrt(2.0f);
    EXPECT_NEAR(g, 1.0f, 0.15f);
    std::printf("test_svf OK\n"); return 0;
}
