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

#include "CircularBuffer.hpp"
using namespace dubwize;

int main() {
    CircularBuffer cb; cb.createCircularBuffer(2000);     // rounds to 2048
    for (int i = 1; i <= 100; ++i) cb.writeBuffer((float) i);  // last written = 100 at newest
    EXPECT_NEAR(cb.readBuffer(1), 100.0f, 1e-6);
    EXPECT_NEAR(cb.readBuffer(2),  99.0f, 1e-6);
    EXPECT_NEAR(cb.readBuffer(2.5f, true), 0.5f*(99.0f+98.0f), 1e-4);
    std::printf("test_circular_buffer OK\n"); return 0;
}
