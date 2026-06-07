// Derived from StrangeReturns by JackWithOneEye
// Original: https://github.com/JackWithOneEye/StrangeReturns
// Licensed under the Apache License, Version 2.0
// Modified by Leo Fabre / Dubplex
// DPF port (header-only): Leo Fabre / Nexus-Preamp
#pragma once

#include <cmath>

#include "DspMath.hpp"
#include "SmoothedValue.hpp"
#include "CircularBuffer.hpp"
#include "SvfFilter.hpp"
#include "Lfo.hpp"
#include "NoiseGenerator.hpp"
#include "DcBlocker.hpp"
#include "BitMod.hpp"
#include "TapeStage.hpp"
#include "DubwizeConstants.h"

namespace dubwize {

class DubwizeEngine
{
public:
    DubwizeEngine() = default;
    ~DubwizeEngine() = default;

    enum class ToneType
    {
        Digital,
        Tape
    };

    enum class NoiseType
    {
        White,
        Brownian,
        Pink
    };

    enum class EffectsRouting
    {
        Loop,
        Wet,
        Input
    };

    enum class FilterPosition
    {
        PreBitmod,
        PostBitmod,
        Feedback
    };

    enum class BitModOperands
    {
        PostFxPostFx,
        PreFxPostFx,
        DryPostFx
    };

    struct DelayParams
    {
        float time_ms = DEFAULT_TIME_MS;
        float feedback_pct = DEFAULT_DELAY_PARAM_FEEDBACK_PCT;
        ToneType toneType = static_cast<ToneType>(DEFAULT_TONE_TYPE_INDEX);
        float tapeDrive = DEFAULT_TAPE_DRIVE;
        float tapeComp_pct = DEFAULT_TAPE_COMP_PCT;
        float tapeAge_pct = DEFAULT_TAPE_AGE_PCT;
        float tapeBias = DEFAULT_TAPE_BIAS;
        float modRate_Hz = DEFAULT_MOD_RATE_HZ;
        float modDepth_pct = DEFAULT_MOD_DEPTH_PCT;
        FastMathLfo::LFOWave modWave = static_cast<FastMathLfo::LFOWave>(DEFAULT_MOD_WAVE_INDEX);
        bool modEnabled = DEFAULT_MOD_ENABLED;
        float noiseLevel_dB = DEFAULT_NOISE_LEVEL_DB;
        NoiseType noiseType = static_cast<NoiseType>(DEFAULT_NOISE_TYPE_INDEX);
        float noiseDucking_pct = DEFAULT_NOISE_DUCKING_PCT;
        bool noiseEnabled = DEFAULT_NOISE_ENABLED;
        float crossFeed_pct = DEFAULT_CROSS_FEED_PCT;
        bool hold = DEFAULT_HOLD_ENABLED;
        float time2_ms = DEFAULT_TIME2_MS;
        bool timeLink = DEFAULT_TIME_LINK_ENABLED;
    };

    struct EffectsParams
    {
        EffectsRouting routing = static_cast<EffectsRouting>(DEFAULT_EFFECTS_ROUTING_INDEX);
        bool flipPhase = DEFAULT_FLIP_PHASE;
        bool decimEnabled = DEFAULT_DECIMATOR_ENABLED;
        float bcDepth_lin = DEFAULT_EFFECTS_PARAM_BC_DEPTH_LIN;
        float decimReduction_lin = DEFAULT_DECIMATOR_RATIO;
        float decimStereoSpread_lin = DEFAULT_DECIMATOR_STEREO_SPREAD;
        bool filterEnabled = DEFAULT_FILTER_ENABLED;
        float lpfCutoff_Hz = DEFAULT_LPF_CUTOFF_HZ;
        float lpfQ_lin = DEFAULT_LPF_Q_LIN;
        FilterPosition lpfPosition = static_cast<FilterPosition>(DEFAULT_LPF_POSITION_INDEX);
        float hpfCutoff_Hz = DEFAULT_HPF_CUTOFF_HZ;
        float hpfQ_lin = DEFAULT_HPF_Q_LIN;
        FilterPosition hpfPosition = static_cast<FilterPosition>(DEFAULT_HPF_POSITION_INDEX);
        bool filterGainComp = DEFAULT_FILTER_GAIN_COMP;
        bool filterSoftClip = DEFAULT_FILTER_SOFT_CLIP;
        bool bmEnabled = DEFAULT_BM_ENABLED;
        float bmLevel_dB = DEFAULT_BM_LEVEL_DB;
        BitMod::Operation bmOperation = static_cast<BitMod::Operation>(DEFAULT_BM_OPERATION_INDEX);
        BitModOperands bmOperands = static_cast<BitModOperands>(DEFAULT_BM_OPERANDS_INDEX);
    };

    void prepare(double sampleRate, int maxBlock)
    {
        fs = (float) sampleRate;

        maxModDepth_smpls = MAX_MOD_DEPTH_SECS * fs;

        time_smpls.reset(fs, 0.25f);
        time2_smpls.reset(fs, 0.25f);
        feedback_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        tapeDrive_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        tapeComp_pct.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        tapeAge_pct.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        tapeBias_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        modRate_Hz.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        modDepth_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        noiseLevel_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        noiseDucking_amt.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        noiseSidechainEnvelope = 0.0f;
        noiseSidechainAttackCoeff = std::exp(-1.0f / (NOISE_DUCKING_ATTACK_SEC * fs));
        noiseSidechainReleaseCoeff = std::exp(-1.0f / (NOISE_DUCKING_RELEASE_SEC * fs));
        noiseWithInputReleaseCoeff = std::exp(-1.0f / (NOISE_WITH_INPUT_RELEASE_SEC * fs));

        crossFeed_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        smoothedPhaseFlip.reset(fs, 0.01f);

        bcDepth_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        decimReduction_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        decimStereoSpread_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        lpfCutoff_Hz.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        lpfQ_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        hpfCutoff_Hz.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);
        hpfQ_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        bmLevel_lin.reset(fs, SMOOTHED_VAL_RAMP_LEN_SEC);

        for (int channel = 0; channel < NUM_CHANNELS; ++channel)
        {
            delayBuffer[channel].createCircularBuffer(static_cast<int>(fs) * MAX_DELAY_TIME_SEC);

            tapeDelayBandpass[channel].reset(fs);
            tapeDelayBandpass[channel].setParameters(TapeStage::ageToCenterFreq(DEFAULT_TAPE_AGE_PCT),
                                                     TapeStage::ageToQ(DEFAULT_TAPE_AGE_PCT),
                                                     false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);

            delayHiPass[channel].reset(fs);
            delayHiPass[channel].setParameters(100.0f, 0.707f, false, false, 0.0f, 0.0f, 1.0f, 0.0f, false);

            modLfo[channel].reset(fs);

            decimPhasor[channel] = 0.0f;
            decimCurrentOutput[channel] = 0.0f;

            hpf[channel].reset(fs);
            hpf[channel].setParameters(hpfCutoff_Hz.getCurrentValue(), hpfQ_lin.getTargetValue(), false, false, 0.0f, 0.0f,
                                       1.0f, 0.0f, false);

            lpf[channel].reset(fs);
            lpf[channel].setParameters(lpfCutoff_Hz.getCurrentValue(), lpfQ_lin.getTargetValue(), false, false, 0.0f, 0.0f,
                                       0.0f, 1.0f, false);

            dcBlocker[channel].reset(fs);
        }

        whiteNoiseGen.reset(fs);
        brownianNoiseGen.reset(fs);
        pinkNoiseGen.reset(fs);
    }

    void process(float* const* buf, int numCh, int numSamples)
    {
        const int numChannels = jmin(numCh, NUM_CHANNELS);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float delay[NUM_CHANNELS] = { time_smpls.getNextValue(), time2_smpls.getNextValue() };
            const float fb = feedback_lin.getNextValue();
            const float tapeDrive = tapeDrive_lin.getNextValue();
            const float tapeComp = tapeComp_pct.getNextValue();
            const float tapeAge = tapeAge_pct.getNextValue();
            const float tapeBias = tapeBias_lin.getNextValue();
            const bool tapeValsSmoothing = tapeDrive_lin.isSmoothing() || tapeComp_pct.isSmoothing() || tapeAge_pct.isSmoothing() || tapeBias_lin.isSmoothing();
            const bool tapeFilterNeedsUpdate = tapeValsSmoothing || tapeFilterParamsChanged;
            tapeFilterParamsChanged = false;

            bool modValsSmoothing = modEnabled && (modRate_Hz.isSmoothing() || modDepth_lin.isSmoothing());
            const float modRate = modRate_Hz.getNextValue();
            const float modDepth = modDepth_lin.getNextValue();

            float inputSidechain = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
                inputSidechain = jmax(inputSidechain, std::abs(buf[channel][sample]));

            auto noiseLvl = noiseLevel_lin.getNextValue();
            const float noiseDucking = noiseDucking_amt.getNextValue();
            float delayNoise = 0.0f;
            const bool noiseOutputOnly = noiseDucking > 0.0f;
            const bool noiseWithInput = noiseDucking < 0.0f;
            const bool noiseInDelayRead = !noiseOutputOnly && !noiseWithInput;
            if (!hold && noiseEnabled && noiseLvl > 0.001f)
            {
                if (noiseType == NoiseType::White)
                {
                    delayNoise = whiteNoiseGen.nextValue();
                }
                else if (noiseType == NoiseType::Brownian)
                {
                    delayNoise = brownianNoiseGen.nextValue();
                }
                else if (noiseType == NoiseType::Pink)
                {
                    delayNoise = pinkNoiseGen.nextValue();
                }

                delayNoise *= noiseLvl;
            }

            float duckingEnvelope = 0.0f;
            if (!noiseOutputOnly)
            {
                const float sidechainTarget = jlimit(0.0f, 1.0f, inputSidechain * NOISE_DUCKING_SIDECHAIN_GAIN);
                const float releaseCoeff = noiseWithInput ? noiseWithInputReleaseCoeff : noiseSidechainReleaseCoeff;
                const float sidechainCoeff = sidechainTarget > noiseSidechainEnvelope
                                                 ? noiseSidechainAttackCoeff
                                                 : releaseCoeff;
                noiseSidechainEnvelope = sidechainTarget + sidechainCoeff * (noiseSidechainEnvelope - sidechainTarget);
                duckingEnvelope = std::pow(noiseSidechainEnvelope, NOISE_DUCKING_ENVELOPE_CURVE);

                if (noiseWithInput)
                    delayNoise *= 1.0f + noiseDucking * (1.0f - duckingEnvelope);
            }

            const float phaseFlipSmoothed = smoothedPhaseFlip.getNextValue();

            const float bcDepth = bcDepth_lin.getNextValue();

            const float decimReduction = decimReduction_lin.getNextValue();
            const float decimStereoSpread = decimStereoSpread_lin.getNextValue();

            const bool hpfValsSmoothing = hpfCutoff_Hz.isSmoothing() || hpfQ_lin.isSmoothing();
            const float hpfCutoff = hpfCutoff_Hz.getNextValue();
            const float hpfQ = hpfQ_lin.getNextValue();

            const bool lpfValsSmoothing = lpfCutoff_Hz.isSmoothing() || lpfQ_lin.isSmoothing();
            const float lpfCutoff = lpfCutoff_Hz.getNextValue();
            const float lpfQ = lpfQ_lin.getNextValue();

            const bool filterParamsChanged = lpfValsSmoothing || hpfValsSmoothing || filterFlagsChanged;
            filterFlagsChanged = false;

            const float bmLevel = bmLevel_lin.getNextValue();

            const float cf = crossFeed_lin.getNextValue();

            float xIn[NUM_CHANNELS] {};
            float dlOut[NUM_CHANNELS] {};
            float dlHpfOut[NUM_CHANNELS] {};
            float wetSidechain = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                xIn[channel] = buf[channel][sample];
                float x = xIn[channel];

                if (modValsSmoothing)
                {
                    modLfo[channel].setParams(modRate, modDepth, modWave, FastMathLfo::LFOPolarity::Unipolar);
                }

                if (filterParamsChanged)
                {
                    lpf[channel].setParameters(lpfCutoff, lpfQ, filterGainComp, filterSoftClip, 0.0f, 0.0f, 0.0f, 1.0f, false);
                    hpf[channel].setParameters(hpfCutoff, hpfQ, filterGainComp, filterSoftClip, 0.0f, 0.0f, 1.0f, 0.0f, false);
                }

                auto modDepthSmpls = modEnabled ? modLfo[channel].getNextSample(0.0f) * maxModDepth_smpls : 0.0f;
                if (hold)
                    modDepthSmpls = 0.0f;

                auto delayLineOut = delayBuffer[channel].readBuffer(delay[channel] + modDepthSmpls);
                if (noiseInDelayRead)
                    delayLineOut += delayNoise;

                if (effectsRouting == EffectsRouting::Loop)
                {
                    delayLineOut = applyEffects(x, delayLineOut, phaseFlipSmoothed, bcDepth, decimReduction, decimStereoSpread, bmLevel, channel);
                }

                if (toneType == ToneType::Tape)
                {
                    if (tapeFilterNeedsUpdate)
                        tapeDelayBandpass[channel].setParameters(TapeStage::ageToCenterFreq(tapeAge), TapeStage::ageToQ(tapeAge),
                                                                 false, false, 0.0f, 1.0f, 0.0f, 0.0f, false);

                    delayLineOut = TapeStage::softClipper(delayLineOut, tapeDrive, tapeBias);
                    delayLineOut = TapeStage::tapeCompressor(delayLineOut, tapeComp);
                    delayLineOut = TAPE_DEL_LOOP_GAIN * tapeDelayBandpass[channel].processSample(delayLineOut);
                }

                dlHpfOut[channel] = delayHiPass[channel].processSample(delayLineOut);
                dlOut[channel] = delayLineOut;
                wetSidechain = jmax(wetSidechain, std::abs(delayLineOut));
            }

            if (noiseOutputOnly)
            {
                const float sidechainInput = jmax(inputSidechain, wetSidechain);
                const float sidechainTarget = jlimit(0.0f, 1.0f, sidechainInput * NOISE_DUCKING_SIDECHAIN_GAIN);
                const float sidechainCoeff = sidechainTarget > noiseSidechainEnvelope
                                                 ? noiseSidechainAttackCoeff
                                                 : noiseSidechainReleaseCoeff;
                noiseSidechainEnvelope = sidechainTarget + sidechainCoeff * (noiseSidechainEnvelope - sidechainTarget);
                duckingEnvelope = std::pow(noiseSidechainEnvelope, NOISE_DUCKING_ENVELOPE_CURVE);
            }

            // Apply filters in feedback path
            if (filterEnabled && lpfPosition == FilterPosition::Feedback)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                    dlHpfOut[channel] = lpf[channel].processSample(dlHpfOut[channel]);
            }
            if (filterEnabled && hpfPosition == FilterPosition::Feedback)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                    dlHpfOut[channel] = hpf[channel].processSample(dlHpfOut[channel]);
            }

            // Write feedback with cross-feed
            if (numChannels == 2 && cf > 0.0f)
            {
                const float self  = 1.0f - cf;
                const float cross = cf;
                const float inputNoise = noiseWithInput ? delayNoise : 0.0f;
                if (hold)
                {
                    delayBuffer[0].writeBuffer(dlOut[0]);
                    delayBuffer[1].writeBuffer(dlOut[1]);
                }
                else
                {
                    delayBuffer[0].writeBuffer(xIn[0] + inputNoise + fb * (self * dlHpfOut[0] + cross * dlHpfOut[1]));
                    delayBuffer[1].writeBuffer(xIn[1] + inputNoise + fb * (self * dlHpfOut[1] + cross * dlHpfOut[0]));
                }
            }
            else
            {
                for (int channel = 0; channel < numChannels; ++channel)
                    delayBuffer[channel].writeBuffer(hold ? dlOut[channel] : xIn[channel] + (noiseWithInput ? delayNoise : 0.0f) + fb * dlHpfOut[channel]);
            }

            // Output
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float out = dlOut[channel];
                if (noiseOutputOnly)
                {
                    out += delayNoise * (1.0f - noiseDucking * duckingEnvelope);
                }
                else if (noiseWithInput)
                {
                    out += delayNoise;
                }

                if (effectsRouting == EffectsRouting::Wet)
                {
                    out = applyEffects(xIn[channel], out, phaseFlipSmoothed, bcDepth, decimReduction, decimStereoSpread, bmLevel, channel);
                }
                buf[channel][sample] = out;
            }
        }
    }

    void processInputEffects(float* const* buf, int numCh, int numSamples)
    {
        const int numChannels = jmin(numCh, NUM_CHANNELS);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float phaseFlipSmoothed = smoothedPhaseFlip.getNextValue();
            const float bcDepth = bcDepth_lin.getNextValue();
            const float decimReduction = decimReduction_lin.getNextValue();
            const float decimStereoSpread = decimStereoSpread_lin.getNextValue();

            const bool hpfValsSmoothing = hpfCutoff_Hz.isSmoothing() || hpfQ_lin.isSmoothing();
            const float hpfCutoff = hpfCutoff_Hz.getNextValue();
            const float hpfQ = hpfQ_lin.getNextValue();

            const bool lpfValsSmoothing = lpfCutoff_Hz.isSmoothing() || lpfQ_lin.isSmoothing();
            const float lpfCutoff = lpfCutoff_Hz.getNextValue();
            const float lpfQ = lpfQ_lin.getNextValue();

            const bool filterParamsChanged = lpfValsSmoothing || hpfValsSmoothing || filterFlagsChanged;
            filterFlagsChanged = false;

            const float bmLevel = bmLevel_lin.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                if (filterParamsChanged)
                {
                    lpf[channel].setParameters(lpfCutoff, lpfQ, filterGainComp, filterSoftClip, 0.0f, 0.0f, 0.0f, 1.0f, false);
                    hpf[channel].setParameters(hpfCutoff, hpfQ, filterGainComp, filterSoftClip, 0.0f, 0.0f, 1.0f, 0.0f, false);
                }

                const float x = buf[channel][sample];
                buf[channel][sample] = applyEffects(x, x, phaseFlipSmoothed, bcDepth, decimReduction, decimStereoSpread, bmLevel, channel);
            }
        }
    }

    void processNoiseOnly(float* const* buf, int numCh, int numSamples)
    {
        const int numChannels = jmin(numCh, NUM_CHANNELS);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSidechain = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
                inputSidechain = jmax(inputSidechain, std::abs(buf[channel][sample]));

            auto noiseLvl = noiseLevel_lin.getNextValue();
            const float noiseDucking = noiseDucking_amt.getNextValue();
            float outputNoise = 0.0f;
            const bool noiseOutputOnly = noiseDucking > 0.0f;
            const bool noiseWithInput = noiseDucking < 0.0f;

            if (!hold && noiseEnabled && noiseLvl > 0.001f)
            {
                if (noiseType == NoiseType::White)
                {
                    outputNoise = whiteNoiseGen.nextValue();
                }
                else if (noiseType == NoiseType::Brownian)
                {
                    outputNoise = brownianNoiseGen.nextValue();
                }
                else if (noiseType == NoiseType::Pink)
                {
                    outputNoise = pinkNoiseGen.nextValue();
                }

                outputNoise *= noiseLvl;
            }

            const float sidechainTarget = jlimit(0.0f, 1.0f, inputSidechain * NOISE_DUCKING_SIDECHAIN_GAIN);
            const float releaseCoeff = noiseWithInput ? noiseWithInputReleaseCoeff : noiseSidechainReleaseCoeff;
            const float sidechainCoeff = sidechainTarget > noiseSidechainEnvelope
                                             ? noiseSidechainAttackCoeff
                                             : releaseCoeff;
            noiseSidechainEnvelope = sidechainTarget + sidechainCoeff * (noiseSidechainEnvelope - sidechainTarget);
            const float duckingEnvelope = std::pow(noiseSidechainEnvelope, NOISE_DUCKING_ENVELOPE_CURVE);

            if (noiseOutputOnly)
                outputNoise *= 1.0f - noiseDucking * duckingEnvelope;
            else if (noiseWithInput)
                outputNoise *= 1.0f + noiseDucking * (1.0f - duckingEnvelope);

            for (int channel = 0; channel < numChannels; ++channel)
                buf[channel][sample] += outputNoise;
        }
    }

    void setDelayParameters(const DelayParams& params)
    {
        time_smpls.setTargetValue(jmax(MIN_DELAY_SMPLS, params.time_ms * 0.001f * fs));
        feedback_lin.setTargetValue(params.feedback_pct * 0.01f);
        toneType = params.toneType;
        tapeDrive_lin.setTargetValue(params.tapeDrive);
        tapeComp_pct.setTargetValue(params.tapeComp_pct);
        tapeAge_pct.setTargetValue(params.tapeAge_pct);
        tapeBias_lin.setTargetValue(params.tapeBias);
        tapeFilterParamsChanged = true;

        modRate_Hz.setTargetValue(jmax(MIN_MOD_RATE_HZ, params.modRate_Hz));
        modDepth_lin.setTargetValue(params.modDepth_pct * 0.01f);
        modWave = params.modWave;
        modEnabled = params.modEnabled;

        if (noiseLevel_dB != params.noiseLevel_dB)
        {
            noiseLevel_dB = params.noiseLevel_dB;
            noiseLevel_lin.setTargetValue(dbToGain(noiseLevel_dB));
        }
        noiseType = params.noiseType;
        noiseDucking_amt.setTargetValue((params.noiseDucking_pct - 50.0f) * 0.02f);
        noiseEnabled = params.noiseEnabled;

        crossFeed_lin.setTargetValue(params.crossFeed_pct * 0.01f);
        hold = params.hold;

        float t2 = params.timeLink ? params.time_ms : params.time2_ms;
        time2_smpls.setTargetValue(jmax(MIN_DELAY_SMPLS, t2 * 0.001f * fs));
    }

    void setEffectsParameters(const EffectsParams& params)
    {
        effectsRouting = params.routing;

        smoothedPhaseFlip.setTargetValue(params.flipPhase ? -1.0f : 1.0f);

        decimEnabled = params.decimEnabled;
        bcDepth_lin.setTargetValue(params.bcDepth_lin);

        decimReduction_lin.setTargetValue(jmax(MIN_DECIMATOR_RATIO, params.decimReduction_lin));
        decimStereoSpread_lin.setTargetValue(params.decimStereoSpread_lin);

        filterEnabled = params.filterEnabled;
        lpfCutoff_Hz.setTargetValue(params.lpfCutoff_Hz);
        lpfQ_lin.setTargetValue(params.lpfQ_lin);
        lpfPosition = params.lpfPosition;

        hpfCutoff_Hz.setTargetValue(params.hpfCutoff_Hz);
        hpfQ_lin.setTargetValue(params.hpfQ_lin);
        hpfPosition = params.hpfPosition;

        if (filterGainComp != params.filterGainComp || filterSoftClip != params.filterSoftClip)
            filterFlagsChanged = true;
        filterGainComp = params.filterGainComp;
        filterSoftClip = params.filterSoftClip;

        bmEnabled = params.bmEnabled;
        if (bmLevel_dB != params.bmLevel_dB) {
            bmLevel_dB = params.bmLevel_dB;
            bmLevel_lin.setTargetValue(dbToGain(bmLevel_dB));
        }
        bmOperation = params.bmOperation;
        bitModOpFunc = BitMod::getOpFunc(bmOperation);
        bmOperands = params.bmOperands;
    }

private:
    float fs = 44100.0f;

    static constexpr float MIN_DELAY_SMPLS = 2.0f;
    static constexpr float MAX_MOD_DEPTH_SECS = 0.02f;
    static constexpr float TAPE_DEL_LOOP_GAIN = 3.98f;

    EffectsRouting effectsRouting = EffectsRouting::Wet;

    // delay
    SmoothedValueMultiplicative time_smpls = SmoothedValueMultiplicative(1.0f);
    SmoothedValueMultiplicative time2_smpls = SmoothedValueMultiplicative(1.0f);
    SmoothedValueLinear feedback_lin = SmoothedValueLinear(0.0f);
    ToneType toneType = ToneType::Digital;
    SmoothedValueLinear tapeDrive_lin = SmoothedValueLinear(DEFAULT_TAPE_DRIVE);
    SmoothedValueLinear tapeComp_pct = SmoothedValueLinear(DEFAULT_TAPE_COMP_PCT);
    SmoothedValueLinear tapeAge_pct = SmoothedValueLinear(DEFAULT_TAPE_AGE_PCT);
    SmoothedValueLinear tapeBias_lin = SmoothedValueLinear(DEFAULT_TAPE_BIAS);
    bool tapeFilterParamsChanged = false;

    CircularBuffer delayBuffer[NUM_CHANNELS];
    StaticSvf tapeDelayBandpass[NUM_CHANNELS];
    StaticSvf delayHiPass[NUM_CHANNELS];

    // modulation
    float maxModDepth_smpls = MAX_MOD_DEPTH_SECS * 44100.0f;
    SmoothedValueMultiplicative modRate_Hz = SmoothedValueMultiplicative(MIN_MOD_RATE_HZ);
    SmoothedValueLinear modDepth_lin = SmoothedValueLinear(0.0f);
    FastMathLfo::LFOWave modWave = FastMathLfo::LFOWave::Tri;
    bool modEnabled = true;

    // cross feed
    SmoothedValueLinear crossFeed_lin = SmoothedValueLinear(0.0f);
    bool hold = false;

    FastMathLfo modLfo[NUM_CHANNELS];

    // noise
    SmoothedValueMultiplicative noiseLevel_lin = SmoothedValueMultiplicative(0.001f);
    float noiseLevel_dB = MIN_GAIN_DB;
    NoiseType noiseType = NoiseType::White;
    bool noiseEnabled = true;
    SmoothedValueLinear noiseDucking_amt = SmoothedValueLinear(0.0f);
    float noiseSidechainEnvelope = 0.0f;
    float noiseSidechainAttackCoeff = 0.0f;
    float noiseSidechainReleaseCoeff = 0.0f;
    float noiseWithInputReleaseCoeff = 0.0f;

    WhiteNoiseGenerator whiteNoiseGen;
    BrownianNoiseGenerator brownianNoiseGen;
    PinkNoiseGenerator pinkNoiseGen;

    // phase
    SmoothedValueLinear smoothedPhaseFlip = SmoothedValueLinear(1.0f);

    // bit crusher
    SmoothedValueLinear bcDepth_lin = SmoothedValueLinear(0.0f);

    // decimator
    bool decimEnabled = true;
    SmoothedValueMultiplicative decimReduction_lin = SmoothedValueMultiplicative(1.0f);
    SmoothedValueLinear decimStereoSpread_lin = SmoothedValueLinear(0.0f);
    float decimPhasor[NUM_CHANNELS] { 0.0f, 0.0f };
    float decimCurrentOutput[NUM_CHANNELS] { 0.0f, 0.0f };

    // filter options
    bool filterEnabled = true;
    bool filterGainComp = false;
    bool filterSoftClip = false;
    bool filterFlagsChanged = false;

    // low pass filter
    SmoothedValueMultiplicative lpfCutoff_Hz = SmoothedValueMultiplicative(MAX_FILTER_CUTOFF_FREQ);
    SmoothedValueLinear lpfQ_lin = SmoothedValueLinear(MIN_FILTER_Q);
    FilterPosition lpfPosition = FilterPosition::PreBitmod;
    StaticSvf lpf[NUM_CHANNELS];

    // high pass filter
    SmoothedValueMultiplicative hpfCutoff_Hz = SmoothedValueMultiplicative(MIN_FILTER_CUTOFF_FREQ);
    SmoothedValueLinear hpfQ_lin = SmoothedValueLinear(MIN_FILTER_Q);
    FilterPosition hpfPosition = FilterPosition::PreBitmod;
    StaticSvf hpf[NUM_CHANNELS];

    // bit modulation
    bool bmEnabled = true;
    SmoothedValueMultiplicative bmLevel_lin = SmoothedValueMultiplicative(0.01f);
    float bmLevel_dB = MIN_GAIN_DB;
    BitMod::Operation bmOperation = BitMod::Operation::None;
    BitModOperands bmOperands = BitModOperands::PostFxPostFx;

    BitMod::OperationFunc bitModOpFunc = BitMod::getOpFunc(BitMod::Operation::None);
    DcBlocker dcBlocker[NUM_CHANNELS];

    float applyEffects(float xDry, float xWet, float phaseFlipSmoothed, float bcDepth, float decimReduction, float decimStereoSpread, float bmLevel, int channel)
    {
        auto y = xWet;

        // phase
        y *= phaseFlipSmoothed;

        if (decimEnabled)
        {
            // bit crusher
            if (bcDepth > MIN_BITCRUSHER_Q)
                y = bcDepth * ((int)(y / bcDepth));

            // decimator
            decimPhasor[channel] += decimReduction;
            auto stereoPhaseShift = channel == 0 ? 0.0f : decimStereoSpread;
            if (decimPhasor[channel] + stereoPhaseShift >= 1.0f)
            {
                decimPhasor[channel] -= 1.0f;
                decimCurrentOutput[channel] = y;
            }
            y = decimCurrentOutput[channel];
        }

        // LPF if PreBitmod
        if (filterEnabled && lpfPosition == FilterPosition::PreBitmod)
        {
            y = lpf[channel].processSample(y);
        }
        // HPF if PreBitmod
        if (filterEnabled && hpfPosition == FilterPosition::PreBitmod)
        {
            y = hpf[channel].processSample(y);
        }

        // bit modulation
        if (bmEnabled && bmOperation != BitMod::Operation::None)
        {
            auto operand1 = 0.0f;
            auto operand2 = 0.0f;

            if (bmOperands == BitModOperands::PostFxPostFx)
            {
                operand1 = operand2 = y;
            }
            else if (bmOperands == BitModOperands::PreFxPostFx)
            {
                operand1 = xWet;
                operand2 = y;
            }
            else if (bmOperands == BitModOperands::DryPostFx)
            {
                operand1 = xDry;
                operand2 = y;
            }
            y = bitModOpFunc(operand1, operand2 * bmLevel);
        }
        if (bmEnabled)
            y = dcBlocker[channel].processSample(y);

        // LPF if PostBitmod
        if (filterEnabled && lpfPosition == FilterPosition::PostBitmod)
        {
            y = lpf[channel].processSample(y);
        }
        // HPF if PostBitmod
        if (filterEnabled && hpfPosition == FilterPosition::PostBitmod)
        {
            y = hpf[channel].processSample(y);
        }

        return y;
    }
};

} // namespace dubwize
