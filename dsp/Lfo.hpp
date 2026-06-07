#pragma once
#include <cmath>
#include <functional>
#include "DspMath.hpp"

namespace dubwize {

constexpr float oneOverTwoPi = 1.0f / kTwoPi;

struct NormalisedPhase
{
    void reset() noexcept { phase = 0.0f; }

    float advance(float increment, float phaseShift = 0.0f) noexcept
    {
        auto offset = phaseShift * oneOverTwoPi;

        auto last = phase;
        auto next = last + increment;

        while ((next + offset) >= 1.0f)
            next -= 1.0f;

        phase = next;
        return last + offset;
    }

    float phase = 0.0f;
};

using LFOWaveFunc = std::function<float(NormalisedPhase& /*phase*/, float /*increment*/, float /*phaseShift*/)>;

static inline float polyBlep(float arg, float increment, float modFactor = 1.0f)
{
    auto incr = modFactor * increment;
    if (arg < incr)
    {
        auto t = arg / (incr);
        return t + t - t * t - 1.0f;
    }

    if (arg > 1.0f - incr)
    {
        auto t = (arg - 1.0f) / (incr);
        return t + t + t * t + 1.0f;
    }

    return 0.0f;
}

class FastMathLfo
{
public:
    FastMathLfo() = default;

    enum class LFOWave
    {
        Sin,
        Tri
    };

    enum class LFOPolarity
    {
        Unipolar,
        Bipolar
    };

    LFOPolarity polarity = LFOPolarity::Unipolar;

    void reset(float sampleRate)
    {
        fs = sampleRate;
        phase.reset();
    }

    void setParams(float freq, float _depth, LFOWave _waveform, LFOPolarity _polarity)
    {
        phaseIncrement = freq / fs;
        depth = _depth;
        waveform = _waveform;
        waveFunc = getWaveFunc(waveform);
        polarity = _polarity;
    }

    float getNextSample(float phaseShift)
    {
        if (phaseIncrement <= 0.0f)
            return 0.0f;

        float bipolarSample = waveFunc(phase, phaseIncrement, phaseShift);

        auto halfDepth = 0.5f * depth;

        if (polarity == LFOPolarity::Unipolar)
        {
            return halfDepth * (bipolarSample + 1.0f);
        }

        // bipolar
        return halfDepth * bipolarSample;
    }

private:
    float fs = 44100.0f;
    float phaseIncrement = 0.0f;
    float depth = 0.0f;
    LFOWave waveform = LFOWave::Sin;
    LFOWaveFunc waveFunc = getWaveFunc(LFOWave::Sin);

    NormalisedPhase phase;

    inline LFOWaveFunc getWaveFunc(LFOWave wave)
    {
        if (wave == LFOWave::Sin)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, 0.25f + phaseShift);

                if (arg < 0.5f)
                    return fastSin(kTwoPi * (arg - 0.25f));

                return 0.0f - fastSin(kTwoPi * (arg - 0.75f));
            };

        if (wave == LFOWave::Tri)
            return [](NormalisedPhase& phase, float increment, float phaseShift)
            {
                auto arg = phase.advance(increment, 0.25f + phaseShift);
                return 1.0f - 2.0f * std::fabs(2.0f * arg - 1.0f);
            };

        return [](NormalisedPhase, float, float) { return 0.0f; };
    }
};

} // namespace dubwize
