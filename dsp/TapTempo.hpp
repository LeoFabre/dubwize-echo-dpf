#pragma once
#include "DspMath.hpp"
#include "DubwizeConstants.h"

namespace dubwize {

class TapTempo
{
public:
    struct TempoParams
    {
        bool hostSyncEnabled;
        bool tapEnabled;
        float tapInterval_ms;
        float tapTimeSnapshot_ms;
        float timeKnob_ms;
        float beatMultiplyFactor;
    };

    struct TapResult
    {
        bool shouldEnable        = false;
        bool shouldDisableHost   = false;
        bool shouldDisable       = false;
        float interval_ms        = 0.0f;
        float snapshot_ms        = 0.0f;
    };

    TapResult processTapButton(bool tapButton, bool tapEnabled, bool hostSyncEnabled,
                               float currentTime_ms, double nowMs)
    {
        TapResult result;

        if (tapButton && !prevTapButtonState_)
        {
            tapButtonDownTime_ = nowMs;
            handleTap(tapEnabled, hostSyncEnabled, currentTime_ms, nowMs, result);
        }
        else if (tapButton && prevTapButtonState_)
        {
            if (tapButtonDownTime_ >= 0.0
                && (nowMs - tapButtonDownTime_) > longPressThreshold_ms)
            {
                result.shouldDisable = true;
                tapCount_ = 0;
                tapButtonDownTime_ = -1.0;
            }
        }
        else
        {
            tapButtonDownTime_ = -1.0;
        }

        prevTapButtonState_ = tapButton;
        return result;
    }

    void updateHostBpm(float bpm)
    {
        if (bpm > 0.0f) lastHostBpm_ = bpm;
    }

    float getHostBeatIntervalMs() const { return 60000.0f / lastHostBpm_; }

    float computeEffectiveTime(const TempoParams& p)
    {
        if (p.hostSyncEnabled)
            return (60000.0f / lastHostBpm_) * p.beatMultiplyFactor;

        if (p.tapEnabled)
        {
            const float offset_ms = p.timeKnob_ms - p.tapTimeSnapshot_ms;
            return jlimit(MIN_DELAY_TIME_MS, MAX_DELAY_TIME_SEC * 1000.0f,
                          p.beatMultiplyFactor * p.tapInterval_ms + offset_ms);
        }

        return p.timeKnob_ms * p.beatMultiplyFactor;
    }

private:
    void handleTap(bool tapEnabled, bool hostSyncEnabled, float currentTime_ms,
                   double nowMs, TapResult& result)
    {
        const double now = nowMs;

        if (tapCount_ > 0 && (now - tapTimes_[tapCount_ - 1]) > tapTimeout_ms)
            tapCount_ = 0;

        if (tapCount_ < maxTapHistory)
            tapTimes_[tapCount_++] = now;
        else
        {
            for (int i = 0; i < maxTapHistory - 1; ++i)
                tapTimes_[i] = tapTimes_[i + 1];
            tapTimes_[maxTapHistory - 1] = now;
        }

        if (tapCount_ >= 2)
        {
            const double totalInterval = tapTimes_[tapCount_ - 1] - tapTimes_[0];
            result.interval_ms = jlimit(MIN_DELAY_TIME_MS, MAX_DELAY_TIME_SEC * 1000.0f,
                                        static_cast<float>(totalInterval / (tapCount_ - 1)));
            result.shouldDisableHost = hostSyncEnabled;
            result.shouldEnable      = !tapEnabled;
            result.snapshot_ms       = currentTime_ms;
        }
    }

    bool   prevTapButtonState_    = false;
    double tapButtonDownTime_     = -1.0;

    static constexpr int    maxTapHistory        = 8;
    static constexpr double tapTimeout_ms        = 3000.0;
    static constexpr double longPressThreshold_ms = 500.0;

    double tapTimes_[maxTapHistory] {};
    int    tapCount_    = 0;
    float  lastHostBpm_ = 120.0f;
};

} // namespace dubwize
