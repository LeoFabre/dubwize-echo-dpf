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

#include "SmoothedValue.hpp"
using namespace dubwize;

int main() {
    SmoothedValueLinear lin;
    lin.reset(100.0f, 0.1f);          // 10 steps
    lin.setCurrentAndTargetValue(0.0f);
    lin.setTargetValue(1.0f);
    EXPECT_TRUE(lin.isSmoothing());
    float prev = 0.0f;
    for (int i = 0; i < 9; ++i) { float v = lin.getNextValue(); EXPECT_TRUE(v > prev); prev = v; }
    float last = lin.getNextValue();
    EXPECT_NEAR(last, 1.0f, 1e-5);
    EXPECT_TRUE(!lin.isSmoothing());
    EXPECT_NEAR(lin.getNextValue(), 1.0f, 1e-9);

    SmoothedValueMultiplicative mul;
    mul.reset(100.0f, 0.1f);
    mul.setCurrentAndTargetValue(1.0f);
    mul.setTargetValue(8.0f);
    for (int i = 0; i < 9; ++i) (void) mul.getNextValue();
    EXPECT_NEAR(mul.getNextValue(), 8.0f, 1e-3);

    SmoothedValueLinear l2; l2.reset(100.0f, 0.1f); l2.setCurrentAndTargetValue(0.5f);
    l2.setTargetValue(0.5f); EXPECT_TRUE(!l2.isSmoothing());
    std::printf("test_smoothed_value OK\n"); return 0;
}
