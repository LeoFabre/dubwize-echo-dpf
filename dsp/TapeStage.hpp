#pragma once
#include <cmath>
#include "DspMath.hpp"
#include "DubwizeConstants.h"

namespace dubwize {

struct TapeStage
{
    static float ageToCenterFreq(float agePct)
    {
        const float age = jlimit(0.0f, 1.0f, agePct * 0.01f);
        constexpr float newTapeHz = 1752.0f;
        constexpr float oldTapeHz = 300.0f;
        return newTapeHz * std::pow(oldTapeHz / newTapeHz, age);
    }

    static float ageToQ(float agePct)
    {
        const float age = jlimit(0.0f, 1.0f, agePct * 0.01f);
        return 0.55f - 0.44f * age;
    }

    static float softClipper(float x, float drive, float bias)
    {
        drive = jlimit(MIN_TAPE_DRIVE, MAX_TAPE_DRIVE, drive);
        bias  = jlimit(MIN_TAPE_BIAS,  MAX_TAPE_BIAS,  bias);

        if (drive <= 0.0001f && std::abs(bias) < 0.0001f)
            return x;

        if (drive <= 0.0001f)
            return x;

        const float biasedZero  = std::tanh(bias * drive);
        const float driven      = std::tanh((x + bias) * drive) - biasedZero;
        const float compensation = std::tanh(drive);

        return compensation > 0.0001f ? driven / compensation : driven;
    }

    // --- Coefficient-cached soft clipper (numerically equivalent to softClipper) ---
    // Hoists the input-independent terms (biasedZero, 1/compensation) out of the
    // per-sample path; recomputed only when bias/drive change via updateCoeffs().
    void updateCoeffs(float drive, float bias)
    {
        drive_ = jlimit(MIN_TAPE_DRIVE, MAX_TAPE_DRIVE, drive);
        bias_  = jlimit(MIN_TAPE_BIAS,  MAX_TAPE_BIAS,  bias);

        passthrough_ = (drive_ <= 0.0001f);

        biasedZero_ = std::tanh(bias_ * drive_);
        const float compensation = std::tanh(drive_);
        compensate_   = compensation > 0.0001f;
        invCompensation_ = compensate_ ? (1.0f / compensation) : 1.0f;
    }

    float softClipperCached(float x) const
    {
        if (passthrough_)
            return x;

        const float driven = std::tanh((x + bias_) * drive_) - biasedZero_;
        return compensate_ ? driven * invCompensation_ : driven;
    }

    static float tapeCompressor(float x, float compPct)
    {
        const float amount = jlimit(0.0f, 1.0f, compPct * 0.01f);
        if (amount <= 0.0001f)
            return x;

        const float threshold = 0.28f + 0.22f * (1.0f - amount);
        const float ratio     = 1.0f + 7.0f * amount;
        const float absX      = std::abs(x);

        if (absX <= threshold)
            return x;

        const float over          = absX - threshold;
        const float compressedAbs = threshold + over / ratio;
        const float compressed    = std::copysign(compressedAbs, x);
        const float makeup        = 1.0f + 0.45f * amount;

        return compressed * makeup;
    }

private:
    // Cached soft-clipper coefficients (see updateCoeffs / softClipperCached).
    float drive_ = 0.0f;
    float bias_  = 0.0f;
    float biasedZero_ = 0.0f;
    float invCompensation_ = 1.0f;
    bool  passthrough_ = true;
    bool  compensate_  = false;
};

} // namespace dubwize
