// test_svf_stereo — StaticSvfStereo must match two scalar StaticSvf (one per
// channel, shared coeffs) sample-for-sample. Reports the max abs difference so
// we know whether it is bit-exact or only ULP-close (e.g. from FMA contraction).
#include "SvfFilter.hpp"
#include <cstdio>
#include <cmath>
#include <cstdint>

using namespace dubwize;

static uint32_t g = 12345u;
static float rnd() { g = 1664525u*g + 1013904223u; return (float)(g>>8)*(1.0f/8388608.0f) - 1.0f; }

struct Cfg { float fc, q; bool gc, sc; float bsf, bpf, hpf, lpf; bool mn; };

int main()
{
    const float fs = 48000.0f;
    const Cfg cfgs[] = {
        {1200.f, 3.0f,  false, false, 0,0,0,1, false},  // LPF (feedback-style)
        { 300.f, 2.0f,  false, false, 0,0,1,0, false},  // HPF
        {2000.f, 0.707f,true,  false, 0,1,0,0, true },  // gainComp + matchNyquist + BPF
        { 800.f, 5.0f,  false, true,  1,0,0,0, false},  // softClip + BSF
    };

    double maxDiff = 0.0;
    for (const auto& c : cfgs) {
        StaticSvf a, b; StaticSvfStereo st;
        a.reset(fs); b.reset(fs); st.reset(fs);
        a.setParameters (c.fc,c.q,c.gc,c.sc,c.bsf,c.bpf,c.hpf,c.lpf,c.mn);
        b.setParameters (c.fc,c.q,c.gc,c.sc,c.bsf,c.bpf,c.hpf,c.lpf,c.mn);
        st.setParameters(c.fc,c.q,c.gc,c.sc,c.bsf,c.bpf,c.hpf,c.lpf,c.mn);

        for (int i = 0; i < 50000; ++i) {
            const float L = rnd(), R = rnd();
            const float la = a.processSample(L);
            const float rb = b.processSample(R);
            float ls = L, rs = R;
            st.processStereo(ls, rs);
            maxDiff = std::fmax(maxDiff, std::fabs((double)la - ls));
            maxDiff = std::fmax(maxDiff, std::fabs((double)rb - rs));
        }
    }

    const double db = maxDiff > 0.0 ? 20.0 * std::log10(maxDiff) : -999.0;
    std::printf("StaticSvfStereo vs 2x StaticSvf: max abs diff = %.3e (%.1f dB)\n", maxDiff, db);
    if (maxDiff == 0.0) {
        std::puts("OK test_svf_stereo (BIT-IDENTICAL)");
        return 0;
    }
    if (db <= -120.0) {
        std::puts("OK test_svf_stereo (equivalent within -120 dB; ULP-level, e.g. FMA contraction)");
        return 0;
    }
    std::puts("FAIL test_svf_stereo (diff exceeds -120 dB)");
    return 1;
}
