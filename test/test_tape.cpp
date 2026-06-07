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

#include "TapeStage.hpp"
using namespace dubwize;
int main() {
    EXPECT_NEAR(TapeStage::softClipper(0.5f, 0.0f, 0.0f), 0.5f, 1e-6);
    float o = TapeStage::softClipper(0.9f, 6.0f, 0.0f);
    EXPECT_TRUE(std::isfinite(o) && std::abs(o) <= 1.2f);

    EXPECT_NEAR(TapeStage::tapeCompressor(0.1f, 80.0f), 0.1f, 1e-6);
    float c = TapeStage::tapeCompressor(0.9f, 80.0f);
    EXPECT_TRUE(std::abs(c) < 0.9f * (1.0f + 0.45f));

    EXPECT_TRUE(TapeStage::ageToCenterFreq(0.0f) > TapeStage::ageToCenterFreq(100.0f));
    EXPECT_TRUE(TapeStage::ageToQ(0.0f) > TapeStage::ageToQ(100.0f));
    std::printf("test_tape OK\n"); return 0;
}
