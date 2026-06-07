#include "DubwizePlugin.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

START_NAMESPACE_DISTRHO

using dubwize::Param;
using dubwize::ParamInfo;
using dubwize::paramInfo;
using dubwize::kNumControlParams;
using dubwize::kNumOutputParams;
using dubwize::DubwizeEngine;
using dubwize::BitMod;
using dubwize::FastMathLfo;

static inline int idx(Param p) { return static_cast<int>(p); }

DubwizePlugin::DubwizePlugin()
    : Plugin(kNumControlParams + kNumOutputParams, 0, 0)
{
    for (int i = 0; i < kNumControlParams; ++i)
        params_[i] = paramInfo(static_cast<Param>(i)).def;
    for (int i = 0; i < kNumOutputParams; ++i)
        outParams_[i] = paramInfo(static_cast<Param>(kNumControlParams + i)).def;
}

void DubwizePlugin::initParameter(uint32_t index, Parameter& parameter)
{
    const ParamInfo& pi = paramInfo(static_cast<Param>(index));

    parameter.name   = pi.name;
    parameter.symbol = pi.symbol;
    parameter.ranges.min = pi.min;
    parameter.ranges.def = pi.def;
    parameter.ranges.max = pi.max;

    parameter.hints = kParameterIsAutomatable;
    if (pi.isBool)        parameter.hints |= kParameterIsBoolean;
    if (pi.isInteger)     parameter.hints |= kParameterIsInteger;
    if (pi.isLogarithmic) parameter.hints |= kParameterIsLogarithmic;
    if (pi.isOutput)      parameter.hints |= kParameterIsOutput;

    if (pi.numChoices > 0)
    {
        parameter.enumValues.count = (uint32_t) pi.numChoices;
        parameter.enumValues.restrictedMode = true;
        auto* ev = new ParameterEnumerationValue[pi.numChoices];
        for (int i = 0; i < pi.numChoices; ++i)
        {
            ev[i].value = (float) i;
            ev[i].label = pi.choices[i];
        }
        parameter.enumValues.values = ev;
    }
}

float DubwizePlugin::getParameterValue(uint32_t index) const
{
    return index < (uint32_t) kNumControlParams
        ? params_[index]
        : outParams_[index - kNumControlParams];
}

void DubwizePlugin::setParameterValue(uint32_t index, float value)
{
    if (index < (uint32_t) kNumControlParams)
    {
        params_[index] = value;
        requiresUpdate_ = true;
    }
}

void DubwizePlugin::activate()
{
    const double fs = getSampleRate();
    const int mb = (int) getBufferSize();
    engine_.prepare(fs, mb);
    ppA_.prepare(fs, mb);
    ppB_.prepare(fs, mb);

    dryL_.assign(mb, 0.0f);
    dryR_.assign(mb, 0.0f);
    ppAL_.assign(mb, 0.0f);
    ppAR_.assign(mb, 0.0f);
    ppBL_.assign(mb, 0.0f);
    ppBR_.assign(mb, 0.0f);

    frameClockMs_ = 0.0;
    requiresUpdate_ = true;
    tapEnabledInternal_ = params_[idx(Param::tapTempoEnabled)] > 0.5f;
}

void DubwizePlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    const double blockStartMs = frameClockMs_;
    const float fs = (float) getSampleRate();

    // 2. Copy inputs -> outputs first; operate on outputs as the working buffer.
    for (int ch = 0; ch < 2; ++ch)
        for (uint32_t i = 0; i < frames; ++i)
            outputs[ch][i] = inputs[ch][i];

    // 3. Capture dry signal.
    const float mixValue = params_[idx(Param::mix)] / 100.0f;
    if (mixValue < 1.0f)
    {
        for (uint32_t i = 0; i < frames; ++i)
        {
            dryL_[i] = outputs[0][i];
            dryR_[i] = outputs[1][i];
        }
    }

    // 4. Host BPM.
    const TimePosition& tp = getTimePosition();
    if (tp.bbt.valid && tp.bbt.beatsPerMinute > 0.0)
        lastHostBpm_ = (float) tp.bbt.beatsPerMinute;
    tapTempo_.updateHostBpm(lastHostBpm_ > 0.0f ? lastHostBpm_ : 120.0f);

    // 5. Tap button.
    auto tr = tapTempo_.processTapButton(
        params_[idx(Param::tapTempoButton)] > 0.5f,
        tapEnabledInternal_,
        params_[idx(Param::hostSyncEnabled)] > 0.5f,
        params_[idx(Param::time)],
        blockStartMs);

    if (tr.shouldDisable)
    {
        tapEnabledInternal_ = false;
        requiresUpdate_ = true;
    }
    else if (tr.interval_ms > 0.0f)
    {
        outParams_[0] = tr.interval_ms;
        outParams_[1] = tr.snapshot_ms;
        if (tr.shouldEnable)
            tapEnabledInternal_ = true;
        requiresUpdate_ = true;
    }

    // 6. Host beat interval output.
    outParams_[2] = tapTempo_.getHostBeatIntervalMs();

    // 7. Beat multiply factor.
    const float beatMultiplyFactor = (float) std::atof(
        paramInfo(Param::beatMultiply).choices[(int) params_[idx(Param::beatMultiply)]]);

    // 8. Recompute DSP parameters if needed.
    if (requiresUpdate_)
    {
        const float effectiveTime = tapTempo_.computeEffectiveTime({
            params_[idx(Param::hostSyncEnabled)] > 0.5f,
            tapEnabledInternal_,
            outParams_[0],
            outParams_[1],
            params_[idx(Param::time)],
            beatMultiplyFactor });

        const float effectiveTime2 = params_[idx(Param::time2)] * beatMultiplyFactor;

        const DubwizeEngine::DelayParams delayParams {
            .time_ms = effectiveTime,
            .feedback_pct = params_[idx(Param::feedback)],
            .toneType = (DubwizeEngine::ToneType)(int) params_[idx(Param::toneType)],
            .tapeDrive = params_[idx(Param::tapeDrive)],
            .tapeComp_pct = params_[idx(Param::tapeComp)],
            .tapeAge_pct = params_[idx(Param::tapeAge)],
            .tapeBias = params_[idx(Param::tapeBias)],
            .modRate_Hz = params_[idx(Param::modRate)],
            .modDepth_pct = params_[idx(Param::modDepth)],
            .modWave = (FastMathLfo::LFOWave)(int) params_[idx(Param::modWave)],
            .modEnabled = params_[idx(Param::modEnabled)] > 0.5f,
            .noiseLevel_dB = params_[idx(Param::noiseLevel)],
            .noiseType = (DubwizeEngine::NoiseType)(int) params_[idx(Param::noiseType)],
            .noiseDucking_pct = params_[idx(Param::noiseDucking)],
            .noiseEnabled = params_[idx(Param::noiseEnabled)] > 0.5f,
            .crossFeed_pct = params_[idx(Param::crossFeed)],
            .hold = params_[idx(Param::hold)] > 0.5f,
            .time2_ms = effectiveTime2,
            .timeLink = params_[idx(Param::timeLink)] > 0.5f,
        };

        const DubwizeEngine::EffectsParams effectsParams {
            .routing = (DubwizeEngine::EffectsRouting)(int) params_[idx(Param::effectsRouting)],
            .flipPhase = params_[idx(Param::flipPhase)] > 0.5f,
            .decimEnabled = params_[idx(Param::decimEnabled)] > 0.5f,
            .bcDepth_lin = params_[idx(Param::bcDepth)],
            .decimReduction_lin = params_[idx(Param::decimReduction)],
            .decimStereoSpread_lin = params_[idx(Param::decimStereoSpread)],
            .filterEnabled = params_[idx(Param::filterEnabled)] > 0.5f,
            .lpfCutoff_Hz = params_[idx(Param::lpfCutoff)],
            .lpfQ_lin = std::max(0.1f, params_[idx(Param::lpfQ)] * 0.707f),
            .lpfPosition = (DubwizeEngine::FilterPosition)(int) params_[idx(Param::lpfPosition)],
            .hpfCutoff_Hz = params_[idx(Param::hpfCutoff)],
            .hpfQ_lin = std::max(0.1f, params_[idx(Param::hpfQ)] * 0.707f),
            .hpfPosition = (DubwizeEngine::FilterPosition)(int) params_[idx(Param::hpfPosition)],
            .filterGainComp = params_[idx(Param::filterGainComp)] > 0.5f,
            .filterSoftClip = params_[idx(Param::filterSoftClip)] > 0.5f,
            .bmEnabled = params_[idx(Param::bmEnabled)] > 0.5f,
            .bmLevel_dB = params_[idx(Param::bmLevel)],
            .bmOperation = (BitMod::Operation)(int) params_[idx(Param::bmOperation)],
            .bmOperands = (DubwizeEngine::BitModOperands)(int) params_[idx(Param::bmOperands)],
        };

        engine_.setDelayParameters(delayParams);
        engine_.setEffectsParameters(effectsParams);

        // Ping-pong A: time1, timeLink=true, cf=100%
        auto ppA = delayParams;
        ppA.crossFeed_pct = 100.0f;
        ppA.timeLink = true;
        ppA_.setDelayParameters(ppA);
        ppA_.setEffectsParameters(effectsParams);

        // Ping-pong B: time2, timeLink=true, cf=100%
        auto ppB = delayParams;
        ppB.crossFeed_pct = 100.0f;
        ppB.time_ms = effectiveTime2;
        ppB.timeLink = true;
        ppB_.setDelayParameters(ppB);
        ppB_.setEffectsParameters(effectsParams);

        requiresUpdate_ = false;
    }

    // 9.
    const float ppValue = params_[idx(Param::pingPong)] / 100.0f;
    const auto routing = (DubwizeEngine::EffectsRouting)(int) params_[idx(Param::effectsRouting)];

    // 10. Input effects routing.
    if (routing == DubwizeEngine::EffectsRouting::Input)
        engine_.processInputEffects(outputs, 2, (int) frames);

    // 11. Delay disabled => noise only + dry/wet + tanh.
    if (params_[idx(Param::delayEnabled)] <= 0.5f)
    {
        engine_.processNoiseOnly(outputs, 2, (int) frames);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* dry = (ch == 0) ? dryL_.data() : dryR_.data();
            for (uint32_t i = 0; i < frames; ++i)
            {
                float out = outputs[ch][i];
                if (mixValue < 1.0f)
                    out = dry[i] + mixValue * (out - dry[i]);
                outputs[ch][i] = std::tanh(out);
            }
        }

        frameClockMs_ = blockStartMs + (double) frames / (double) fs * 1000.0;
        return;
    }

    // 12. Prepare ping-pong inputs: mono sum on L, silence on R.
    for (uint32_t i = 0; i < frames; ++i)
    {
        const float mono = (outputs[0][i] + outputs[1][i]) * 0.5f;
        ppAL_[i] = mono;
        ppAR_[i] = 0.0f;
        ppBL_[i] = mono;
        ppBR_[i] = 0.0f;
    }

    engine_.process(outputs, 2, (int) frames);
    {
        float* a[2] = { ppAL_.data(), ppAR_.data() };
        ppA_.process(a, 2, (int) frames);
    }
    {
        float* b[2] = { ppBL_.data(), ppBR_.data() };
        ppB_.process(b, 2, (int) frames);
    }

    // Blend normal and ping-pong outputs.
    if (ppValue > 0.0f)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* ppA = (ch == 0) ? ppAL_.data() : ppAR_.data();
            const float* ppB = (ch == 0) ? ppBL_.data() : ppBR_.data();
            for (uint32_t i = 0; i < frames; ++i)
                outputs[ch][i] = (1.0f - ppValue) * outputs[ch][i]
                               + ppValue * 0.5f * (ppA[i] + ppB[i]);
        }
    }

    // 13. Dry/wet blend + output protection (tanh limiter).
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* dry = (ch == 0) ? dryL_.data() : dryR_.data();
        for (uint32_t i = 0; i < frames; ++i)
        {
            float out = outputs[ch][i];
            if (mixValue < 1.0f)
                out = dry[i] + mixValue * (out - dry[i]);
            outputs[ch][i] = std::tanh(out);
        }
    }

    frameClockMs_ = blockStartMs + (double) frames / (double) fs * 1000.0;
}

Plugin* createPlugin() { return new DubwizePlugin(); }

END_NAMESPACE_DISTRHO
