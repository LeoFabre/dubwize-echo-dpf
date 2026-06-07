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
#include "DubwizeEngine.hpp"
#include <vector>
using namespace dubwize;

int main() {
    const float fs = 48000.0f;
    DubwizeEngine e; e.prepare(fs, 512);
    DubwizeEngine::DelayParams dp;
    dp.time_ms = 100.0f; dp.feedback_pct = 50.0f; dp.noiseEnabled = false; dp.modDepth_pct = 0.0f;
    DubwizeEngine::EffectsParams ep;
    e.setDelayParameters(dp); e.setEffectsParameters(ep);

    // Let the multiplicative time-smoothing ramp (0.25 s) settle before the impulse,
    // so the delay tap sits at the requested 100 ms (matches a settled JUCE original).
    { std::vector<float> s(16384, 0.0f), s2(16384, 0.0f); float* sc[2] = { s.data(), s2.data() }; e.process(sc, 2, 16384); }

    const int N = (int) (fs * 0.5f);
    std::vector<float> L(N, 0.0f), R(N, 0.0f);
    L[0] = R[0] = 1.0f;
    { float* ch[2] = { L.data(), R.data() }; e.process(ch, 2, N); }

    for (int i = 0; i < N; ++i) { EXPECT_TRUE(std::isfinite(L[i])); EXPECT_TRUE(std::isfinite(R[i])); }
    int d = (int) (0.1f * fs);
    float around = 0.0f; for (int i = d - 50; i < d + 50; ++i) around = std::max(around, std::abs(L[i]));
    EXPECT_TRUE(around > 0.05f);

    DubwizeEngine e2; e2.prepare(fs, 512);
    DubwizeEngine::DelayParams dp2; dp2.noiseEnabled = false;
    e2.setDelayParameters(dp2); e2.setEffectsParameters(ep);
    std::vector<float> Z(256, 0.0f), Z2(256, 0.0f); float* zch[2] = { Z.data(), Z2.data() };
    e2.processNoiseOnly(zch, 2, 256);
    float zmax = 0.0f; for (float v : Z) zmax = std::max(zmax, std::abs(v));
    EXPECT_NEAR(zmax, 0.0f, 1e-6);

    for (int routing = 0; routing < 3; ++routing) {
        DubwizeEngine er; er.prepare(fs, 512);
        DubwizeEngine::DelayParams d3; d3.feedback_pct = 70.0f; d3.toneType = DubwizeEngine::ToneType::Tape;
        DubwizeEngine::EffectsParams e3; e3.routing = (DubwizeEngine::EffectsRouting) routing;
        e3.bmEnabled = true; e3.bmOperation = BitMod::Operation::Xor;
        er.setDelayParameters(d3); er.setEffectsParameters(e3);
        std::vector<float> a(1000), b(1000); for (int i=0;i<1000;++i){a[i]=0.3f*std::sin(i*0.1f);b[i]=-a[i];}
        float* c[2]={a.data(),b.data()}; er.process(c,2,1000);
        for (int i=0;i<1000;++i){ EXPECT_TRUE(std::isfinite(a[i])); EXPECT_TRUE(std::isfinite(b[i])); }
    }
    std::printf("test_engine_offline OK\n"); return 0;
}
