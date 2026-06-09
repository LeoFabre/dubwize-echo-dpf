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

    // --- Coefficient-cached soft clipper (BIT-IDENTICAL to softClipper) ---
    // The input-independent terms (biasedZero, compensation) depend only on
    // drive/bias. We recompute them only when drive/bias actually change and
    // reuse them otherwise — reusing a deterministic tanh of unchanged inputs is
    // identical to recomputing it. We keep the SAME clamping, the SAME passthrough
    // conditions, and DIVISION by compensation (not a reciprocal-multiply), so
    // softClipperCached(x) == softClipper(x, drive, bias) for every sample. This
    // is a pure speed optimization (skips the 2 invariant tanh when params are
    // static — the common case) with no output change.
    //
    // NOTE: a previous version gated the refresh on "smoothing" and multiplied by
    // 1/compensation, which let biasedZero_ go stale at steady state and injected
    // a constant offset into the feedback loop (a ~-14 dB regression). Do not
    // reintroduce a smoothing gate here — change detection is what keeps it both
    // correct and cheap.
    void updateCoeffs(float drive, float bias)
    {
        if (coeffsValid_ && drive == lastDrive_ && bias == lastBias_)
            return;
        lastDrive_ = drive;
        lastBias_  = bias;
        coeffsValid_ = true;

        drive_ = jlimit(MIN_TAPE_DRIVE, MAX_TAPE_DRIVE, drive);
        bias_  = jlimit(MIN_TAPE_BIAS,  MAX_TAPE_BIAS,  bias);

        passthrough_  = (drive_ <= 0.0001f);
        biasedZero_   = std::tanh(bias_ * drive_);
        compensation_ = std::tanh(drive_);
        compensate_   = compensation_ > 0.0001f;
    }

    // Force the next updateCoeffs() to recompute (call on engine reset).
    void invalidateCoeffs() { coeffsValid_ = false; }

    float softClipperCached(float x) const
    {
        if (passthrough_)
            return x;

        const float driven = std::tanh((x + bias_) * drive_) - biasedZero_;
        return compensate_ ? driven / compensation_ : driven;
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
    float compensation_ = 1.0f;
    bool  passthrough_ = true;
    bool  compensate_  = false;
    // Change detection: recompute coeffs only when drive/bias differ from last.
    float lastDrive_ = 0.0f;
    float lastBias_  = 0.0f;
    bool  coeffsValid_ = false;
};

} // namespace dubwize
