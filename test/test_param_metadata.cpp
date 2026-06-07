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

#include "ParameterMetadata.hpp"
using namespace dubwize;

int main() {
    EXPECT_TRUE(kNumControlParams == 45);
    EXPECT_TRUE(kNumOutputParams == 3);
    const auto& mix = paramInfo(Param::mix);
    EXPECT_NEAR(mix.min, 0.0f, 1e-6); EXPECT_NEAR(mix.max, 100.0f, 1e-6); EXPECT_NEAR(mix.def, 50.0f, 1e-6);
    const auto& tt = paramInfo(Param::toneType);
    EXPECT_TRUE(tt.isInteger && tt.numChoices == 2);
    const auto& bm = paramInfo(Param::beatMultiply);
    EXPECT_NEAR(bm.def, 5.0f, 1e-6); EXPECT_TRUE(bm.numChoices == 9);
    // a boolean
    const auto& hold = paramInfo(Param::hold);
    EXPECT_TRUE(hold.isBool); EXPECT_NEAR(hold.min, 0.0f, 1e-6); EXPECT_NEAR(hold.max, 1.0f, 1e-6);
    // an output param
    const auto& oi = paramInfo(Param::outTapInterval);
    EXPECT_TRUE(oi.isOutput);
    std::printf("test_param_metadata OK\n"); return 0;
}
