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

#include "TapTempo.hpp"
using namespace dubwize;

int main() {
    TapTempo tt;
    TapTempo::TapResult r;
    auto tap = [&](double nowMs){ return tt.processTapButton(true,  false, false, 500.0f, nowMs); };
    auto rel = [&](double nowMs){ return tt.processTapButton(false, false, false, 500.0f, nowMs); };
    rel(0); tap(0);   rel(10);
    tap(500);  rel(510);
    tap(1000); rel(1010);
    r = tap(1500); rel(1510);
    EXPECT_NEAR(r.interval_ms, 500.0f, 1.0f);
    EXPECT_TRUE(r.shouldEnable);

    TapTempo h; h.updateHostBpm(120.0f);
    EXPECT_NEAR(h.getHostBeatIntervalMs(), 500.0f, 1e-3);
    TapTempo::TempoParams p{}; p.hostSyncEnabled = true; p.beatMultiplyFactor = 1.0f;
    EXPECT_NEAR(h.computeEffectiveTime(p), 500.0f, 1e-3);

    TapTempo lp; lp.processTapButton(false,true,false,500.0f,0.0);
    lp.processTapButton(true, true,false,500.0f,0.0);
    auto dr = lp.processTapButton(true,true,false,500.0f,600.0);
    EXPECT_TRUE(dr.shouldDisable);
    std::printf("test_tap_tempo OK\n"); return 0;
}
