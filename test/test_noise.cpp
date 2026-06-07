// [macro block]
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
#include "NoiseGenerator.hpp"
#include <vector>
using namespace dubwize;
int main() {
    WhiteNoiseGenerator w; w.reset(48000.0f);
    double sum = 0; float mn = 1e9f, mx = -1e9f;
    for (int i = 0; i < 200000; ++i) { float v = w.nextValue(); sum += v; mn = std::min(mn,v); mx = std::max(mx,v); }
    EXPECT_NEAR(sum / 200000.0, 0.0, 0.02);
    EXPECT_TRUE(mn >= -1.0001f && mx <= 1.0001f);

    PinkNoiseGenerator p; p.reset(48000.0f);
    for (int i = 0; i < 100000; ++i) EXPECT_TRUE(std::isfinite(p.nextValue()));

    BrownianNoiseGenerator b; b.reset(48000.0f);
    float bmax = 0.0f; for (int i = 0; i < 100000; ++i) bmax = std::max(bmax, std::abs(b.nextValue()));
    EXPECT_TRUE(bmax <= 0.81f);
    std::printf("test_noise OK\n"); return 0;
}
