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
#include "BitMod.hpp"
using namespace dubwize;
int main() {
    auto none = BitMod::getOpFunc(BitMod::Operation::None);
    EXPECT_NEAR(none(0.37f, 0.91f), 0.37f, 1e-9);          // pass-through of operand a
    auto xorf = BitMod::getOpFunc(BitMod::Operation::Xor);
    EXPECT_NEAR(xorf(0.0f, 0.5f), 0.5f, 1e-9);             // xor with +0 returns b
    auto orf = BitMod::getOpFunc(BitMod::Operation::Or);
    float r = orf(0.5f, 0.5f); EXPECT_TRUE(std::isfinite(r));
    auto andf = BitMod::getOpFunc(BitMod::Operation::And);
    EXPECT_NEAR(andf(0.0f, 0.7f), 0.0f, 1e-9);             // and with +0 returns 0
    std::printf("test_bitmod OK\n"); return 0;
}
