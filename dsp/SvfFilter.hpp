// Derived from StrangeReturns by JackWithOneEye
// Original: https://github.com/JackWithOneEye/StrangeReturns
// Licensed under the Apache License, Version 2.0
// Modified by Leo Fabre / Dubplex
// DPF port (header-only): Leo Fabre / Nexus-Preamp
#pragma once
#include <cmath>
#include "DspMath.hpp"
#include "SmoothedValue.hpp"
#include "SimdF.hpp"

namespace dubwize {

static constexpr float kSmoothRampSec = 0.1f;

// ---------------------------------------------------------------------------
// File-scope helper (mirrors VASVFilter.cpp)
// ---------------------------------------------------------------------------
static inline float peakGainForQ(float q)
{
    if (q <= 0.707f)
        return 1.0f;
    auto q2 = q * q;
    return q2 / std::pow(q2 - 0.25f, 0.5f);
}

// ---------------------------------------------------------------------------
// StaticSvf — was StaticVASVFilter
// ---------------------------------------------------------------------------
class StaticSvf
{
public:
    StaticSvf() = default;

    void reset(float sampleRate)
    {
        fs  = sampleRate;
        sn_1 = 0.0f;
        sn_2 = 0.0f;
    }

    void setParameters(float _fc, float _q,
                       bool _enableGainComp, bool _enableSoftClipper,
                       float _bsfMix, float _bpfMix, float _hpfMix, float _lpfMix,
                       bool _matchAnalogNyquistLPF)
    {
        fc  = _fc;
        q   = _q;
        enableGainComp    = _enableGainComp;
        enableSoftClipper = _enableSoftClipper;

        bsfMix = _bsfMix;
        bpfMix = _bpfMix;
        hpfMix = _hpfMix;
        lpfMix = _lpfMix;

        matchAnalogNyquistLPF = _matchAnalogNyquistLPF;

        calcCoeffs();
    }

    float processSample(float x)
    {
        if (enableGainComp)
            x *= halfPeak;

        auto hpf = alpha_0 * (x - rho * sn_1 - sn_2);
        auto bpf = alpha * hpf + sn_1;
        if (enableSoftClipper)
            bpf = std::tanh(bpf);

        auto lpf  = alpha * bpf + sn_2;
        auto bsf  = hpf + lpf;
        auto lpf2 = matchAnalogNyquistLPF ? lpf + sigma * sn_1 : lpf;

        sn_1 = alpha * hpf + bpf;
        sn_2 = alpha * bpf + lpf;

        return bsfMix * bsf + bpfMix * bpf + hpfMix * hpf + lpfMix * lpf2;
    }

private:
    float fs = 44100.0f;

    float fc = 1000.0f;
    float q  = 0.707f;
    bool  enableGainComp    = false;
    bool  enableSoftClipper = false;

    float halfPeak = 1.0f;
    float alpha    = 0.0f;
    float alpha_0  = 0.0f;
    float r        = 0.707f;
    float rho      = 1.414f;
    float sigma    = 0.0f;

    float sn_1 = 0.0f;
    float sn_2 = 0.0f;

    float bsfMix = 0.0f;
    float bpfMix = 0.0f;
    float hpfMix = 0.0f;
    float lpfMix = 0.0f;

    bool matchAnalogNyquistLPF = true;

    void calcCoeffs()
    {
        alpha   = fastTan((kPi * fc) / fs);
        r       = 1.0f / (2.0f * q);
        rho     = 2.0f * r + alpha;
        alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
        sigma   = (4.0f * fc * fc) / (alpha * fs * fs);

        halfPeak = 1.0f;
        auto peak_dB = gainToDb(peakGainForQ(q));
        if (peak_dB > 0.0f)
            halfPeak = dbToGain(-peak_dB * 0.5f);
    }
};

// ---------------------------------------------------------------------------
// StaticSvfStereo — StaticSvf processing BOTH channels in 2 SIMD lanes.
// Coefficients are shared (scalar); state is per-lane. Each lane runs the exact
// same IEEE +/-/* recurrence as a per-channel StaticSvf, so the result is
// bit-identical to two StaticSvf sharing coeffs. Built on SimdF, so the host
// SSE2 backend exercises the same vector path the Bela runs under NEON; the
// scalar tanh soft-clip (rare) is applied per lane to stay bit-identical.
// Lanes 2-3 are unused — the win is halving the per-sample filter arithmetic.
// ---------------------------------------------------------------------------
class StaticSvfStereo
{
public:
    StaticSvfStereo() = default;

    void reset(float sampleRate)
    {
        fs = sampleRate;
        sn_1[0] = sn_1[1] = 0.0f;
        sn_2[0] = sn_2[1] = 0.0f;
    }

    void setParameters(float _fc, float _q,
                       bool _enableGainComp, bool _enableSoftClipper,
                       float _bsfMix, float _bpfMix, float _hpfMix, float _lpfMix,
                       bool _matchAnalogNyquistLPF)
    {
        fc = _fc; q = _q;
        enableGainComp    = _enableGainComp;
        enableSoftClipper = _enableSoftClipper;
        bsfMix = _bsfMix; bpfMix = _bpfMix; hpfMix = _hpfMix; lpfMix = _lpfMix;
        matchAnalogNyquistLPF = _matchAnalogNyquistLPF;
        calcCoeffs();
    }

    // Process L and R in lanes 0,1 (in place). Bit-identical to two StaticSvf.
    void processStereo(float& l, float& r)
    {
        float xin[4] = { l, r, 0.0f, 0.0f };
        if (enableGainComp) { xin[0] *= halfPeak; xin[1] *= halfPeak; }

        float s1f[4] = { sn_1[0], sn_1[1], 0.0f, 0.0f };
        float s2f[4] = { sn_2[0], sn_2[1], 0.0f, 0.0f };
        const SimdF x  = SimdF::load(xin);
        const SimdF s1 = SimdF::load(s1f);
        const SimdF s2 = SimdF::load(s2f);
        const SimdF A(alpha), A0(alpha_0), RHO(rho), SIG(sigma);

        const SimdF hpf = A0 * (x - RHO * s1 - s2);
        SimdF bpf = A * hpf + s1;
        if (enableSoftClipper) {
            float b[4]; bpf.store(b);
            b[0] = std::tanh(b[0]); b[1] = std::tanh(b[1]);
            bpf = SimdF::load(b);
        }
        const SimdF lpf  = A * bpf + s2;
        const SimdF bsf  = hpf + lpf;
        const SimdF lpf2 = matchAnalogNyquistLPF ? (lpf + SIG * s1) : lpf;

        const SimdF n1 = A * hpf + bpf;
        const SimdF n2 = A * bpf + lpf;
        const SimdF outv = SimdF(bsfMix) * bsf + SimdF(bpfMix) * bpf
                         + SimdF(hpfMix) * hpf + SimdF(lpfMix) * lpf2;

        float o[4], ns1[4], ns2[4];
        outv.store(o); n1.store(ns1); n2.store(ns2);
        sn_1[0] = ns1[0]; sn_1[1] = ns1[1];
        sn_2[0] = ns2[0]; sn_2[1] = ns2[1];
        l = o[0]; r = o[1];
    }

    // Process ONE lane (channel) scalar, advancing only that lane's state.
    // Byte-for-byte StaticSvf::processSample with the shared coeffs and this
    // lane's sn_1/sn_2 — used where only one channel is available at a call
    // site (applyEffects' per-channel positions), so the same unified object
    // holds the state whether a sample goes through processStereo or here.
    float processLane(int lane, float x)
    {
        if (enableGainComp)
            x *= halfPeak;

        float& s1 = sn_1[lane];
        float& s2 = sn_2[lane];

        auto hpf = alpha_0 * (x - rho * s1 - s2);
        auto bpf = alpha * hpf + s1;
        if (enableSoftClipper)
            bpf = std::tanh(bpf);

        auto lpf  = alpha * bpf + s2;
        auto bsf  = hpf + lpf;
        auto lpf2 = matchAnalogNyquistLPF ? lpf + sigma * s1 : lpf;

        s1 = alpha * hpf + bpf;
        s2 = alpha * bpf + lpf;

        return bsfMix * bsf + bpfMix * bpf + hpfMix * hpf + lpfMix * lpf2;
    }

private:
    float fs = 44100.0f;
    float fc = 1000.0f;
    float q  = 0.707f;
    bool  enableGainComp    = false;
    bool  enableSoftClipper = false;

    float halfPeak = 1.0f;
    float alpha    = 0.0f;
    float alpha_0  = 0.0f;
    float r        = 0.707f;
    float rho      = 1.414f;
    float sigma    = 0.0f;

    float sn_1[2] = { 0.0f, 0.0f };
    float sn_2[2] = { 0.0f, 0.0f };

    float bsfMix = 0.0f;
    float bpfMix = 0.0f;
    float hpfMix = 0.0f;
    float lpfMix = 0.0f;

    bool matchAnalogNyquistLPF = true;

    void calcCoeffs()
    {
        alpha   = fastTan((kPi * fc) / fs);
        r       = 1.0f / (2.0f * q);
        rho     = 2.0f * r + alpha;
        alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
        sigma   = (4.0f * fc * fc) / (alpha * fs * fs);

        halfPeak = 1.0f;
        auto peak_dB = gainToDb(peakGainForQ(q));
        if (peak_dB > 0.0f)
            halfPeak = dbToGain(-peak_dB * 0.5f);
    }
};

// ---------------------------------------------------------------------------
// Svf — was VASVFilter
// ---------------------------------------------------------------------------
class Svf
{
public:
    Svf() = default;

    void reset(float sampleRate)
    {
        fs = sampleRate;
        fc.reset(fs, kSmoothRampSec);
        q.reset(fs, kSmoothRampSec);

        bpfMix.reset(fs, kSmoothRampSec);
        bsfMix.reset(fs, kSmoothRampSec);
        hpfMix.reset(fs, kSmoothRampSec);
        lpfMix.reset(fs, kSmoothRampSec);

        sn_1 = 0.0f;
        sn_2 = 0.0f;

        calcCoeffs(true);
    }

    void setParameters(float _fc, float _q,
                       float _bpfMix, float _bsfMix, float _hpfMix, float _lpfMix)
    {
        fc.setTargetValue(_fc);
        q.setTargetValue(_q);

        bpfMix.setTargetValue(_bpfMix);
        bsfMix.setTargetValue(_bsfMix);
        hpfMix.setTargetValue(_hpfMix);
        lpfMix.setTargetValue(_lpfMix);
    }

    float processSample(float x)
    {
        calcCoeffs();

        auto hpf  = alpha_0 * (x - rho * sn_1 - sn_2);
        auto bpf  = alpha * hpf + sn_1;
        auto lpf  = alpha * bpf + sn_2;
        auto bsf  = hpf + lpf;
        auto lpf2 = lpf + sigma * sn_1;

        sn_1 = alpha * hpf + bpf;
        sn_2 = alpha * bpf + lpf;

        return bpfMix.getNextValue() * bpf
             + bsfMix.getNextValue() * bsf
             + hpfMix.getNextValue() * hpf
             + lpfMix.getNextValue() * lpf2;
    }

private:
    float fs = 44100.0f;

    float currentFc = 0.0f;
    SmoothedValueMultiplicative fc{1000.0f};

    float currentQ = 0.0f;
    SmoothedValueLinear q{0.707f};

    float halfPeak = 1.0f;
    float alpha    = 0.0f;
    float alpha_0  = 0.0f;
    float r        = 0.707f;
    float rho      = 1.414f;
    float sigma    = 0.0f;

    SmoothedValueLinear bsfMix{0.0f};
    SmoothedValueLinear bpfMix{0.0f};
    SmoothedValueLinear hpfMix{0.0f};
    SmoothedValueLinear lpfMix{0.0f};

    float sn_1 = 0.0f;
    float sn_2 = 0.0f;

    void calcCoeffs(bool force = false)
    {
        if (!force && !fc.isSmoothing() && !q.isSmoothing())
            return;

        if (force || fc.isSmoothing())
        {
            auto currentFc = fc.getNextValue();
            alpha  = fastTan((kPi * currentFc) / fs);
            sigma  = (4.0f * currentFc * currentFc) / (alpha * fs * fs);
        }

        if (force || q.isSmoothing())
        {
            auto currentQ = q.getNextValue();
            auto peak_dB  = gainToDb(peakGainForQ(currentQ));
            if (peak_dB > 0.0f)
                halfPeak = dbToGain(-peak_dB * 0.5f);

            r = 1.0f / (2.0f * currentQ);
        }

        rho     = 2.0f * r + alpha;
        alpha_0 = 1.0f / (1.0f + 2.0f * r * alpha + alpha * alpha);
    }
};

} // namespace dubwize
