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

#include "DspMath.hpp"
using namespace dubwize;

int main() {
    EXPECT_NEAR(dbToGain(0.0f), 1.0f, 1e-6);
    EXPECT_NEAR(gainToDb(1.0f), 0.0f, 1e-6);
    EXPECT_NEAR(dbToGain(-6.0206f), 0.5f, 1e-4);

    EXPECT_NEAR(jlimit(0.0f, 10.0f, -3.0f), 0.0f, 1e-9);
    EXPECT_NEAR(jlimit(0.0f, 10.0f, 99.0f), 10.0f, 1e-9);

    EXPECT_TRUE(nextPowerOfTwo(1000) == 1024);
    EXPECT_TRUE(nextPowerOfTwo(1024) == 1024);
    EXPECT_TRUE(nextPowerOfTwo(1025) == 2048);

    for (float x = -1.5f; x <= 1.5f; x += 0.1f) {
        float x2 = x*x;
        float num = x*(-135135.0f + x2*(17325.0f + x2*(-378.0f + x2)));
        float den = -135135.0f + x2*(62370.0f + x2*(-3150.0f + 28.0f*x2));
        EXPECT_NEAR(fastTan(x), num/den, 1e-6);
    }
    for (float x = -3.1f; x <= 3.1f; x += 0.1f) {
        float x2 = x*x;
        float num = -x*(-11511339840.0f + x2*(1640635920.0f + x2*(-52785432.0f + x2*479249.0f)));
        float den =     11511339840.0f + x2*( 277920720.0f + x2*(   3177720.0f + x2* 18361.0f));
        EXPECT_NEAR(fastSin(x), num/den, 1e-3);
    }
    std::printf("test_dsp_math OK\n");
    return 0;
}
