#include <algorithm>
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

#include "DcBlocker.hpp"
using namespace dubwize;
int main() {
    DcBlocker dc; dc.reset(48000.0f);
    float y = 0.0f; for (int i = 0; i < 20000; ++i) y = dc.processSample(1.0f);
    EXPECT_NEAR(y, 0.0f, 1e-2);
    DcBlocker dc2; dc2.reset(48000.0f);
    float maxAbs = 0.0f;
    for (int i = 0; i < 2000; ++i) { float x = (i % 2 == 0) ? 1.0f : -1.0f; float o = dc2.processSample(x); if (i>10) maxAbs = std::max(maxAbs, std::abs(o)); }
    EXPECT_TRUE(maxAbs > 0.9f);
    std::printf("test_dc_blocker OK\n"); return 0;
}
