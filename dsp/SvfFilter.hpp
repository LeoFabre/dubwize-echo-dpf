// Derived from StrangeReturns by JackWithOneEye
// Original: https://github.com/JackWithOneEye/StrangeReturns
// Licensed under the Apache License, Version 2.0
// Modified by Leo Fabre / Dubplex
// DPF port (header-only): Leo Fabre / Nexus-Preamp
#pragma once
#include <cmath>
#include "DspMath.hpp"
#include "SmoothedValue.hpp"

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
