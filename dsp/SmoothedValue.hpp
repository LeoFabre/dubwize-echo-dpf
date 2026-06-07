#pragma once
#include <cmath>

namespace dubwize {

class SmoothedValueLinear {
public:
    SmoothedValueLinear() = default;
    explicit SmoothedValueLinear(float initial) : currentValue_(initial), target_(initial) {}

    void reset(float sampleRate, float rampSeconds) noexcept {
        stepsToTarget_ = (int) std::floor(rampSeconds * sampleRate);
        countdown_ = 0; currentValue_ = target_;
    }
    void setCurrentAndTargetValue(float v) noexcept { currentValue_ = target_ = v; countdown_ = 0; }
    void setTargetValue(float v) noexcept {
        if (v == target_) return;
        if (stepsToTarget_ <= 0) { setCurrentAndTargetValue(v); return; }
        target_ = v; countdown_ = stepsToTarget_;
        step_ = (target_ - currentValue_) / (float) countdown_;
    }
    float getNextValue() noexcept {
        if (countdown_ <= 0) return target_;
        --countdown_;
        if (countdown_ == 0) currentValue_ = target_;
        else currentValue_ += step_;
        return currentValue_;
    }
    bool  isSmoothing()     const noexcept { return countdown_ > 0; }
    float getCurrentValue() const noexcept { return currentValue_; }
    float getTargetValue()  const noexcept { return target_; }
private:
    float currentValue_ = 0.0f, target_ = 0.0f, step_ = 0.0f;
    int stepsToTarget_ = 0, countdown_ = 0;
};

class SmoothedValueMultiplicative {
public:
    SmoothedValueMultiplicative() = default;
    explicit SmoothedValueMultiplicative(float initial)
        : currentValue_(initial == 0.0f ? 1e-9f : initial), target_(currentValue_) {}

    void reset(float sampleRate, float rampSeconds) noexcept {
        stepsToTarget_ = (int) std::floor(rampSeconds * sampleRate);
        countdown_ = 0;
    }
    void setCurrentAndTargetValue(float v) noexcept {
        currentValue_ = target_ = (v == 0.0f ? 1e-9f : v); countdown_ = 0;
    }
    void setTargetValue(float v) noexcept {
        v = (v == 0.0f ? 1e-9f : v);
        if (v == target_) return;
        if (stepsToTarget_ <= 0) { setCurrentAndTargetValue(v); return; }
        target_ = v; countdown_ = stepsToTarget_;
        step_ = std::exp((std::log(std::abs(target_)) - std::log(std::abs(currentValue_))) / (float) countdown_);
    }
    float getNextValue() noexcept {
        if (countdown_ <= 0) return target_;
        --countdown_;
        if (countdown_ == 0) currentValue_ = target_;
        else currentValue_ *= step_;
        return currentValue_;
    }
    bool  isSmoothing()     const noexcept { return countdown_ > 0; }
    float getCurrentValue() const noexcept { return currentValue_; }
    float getTargetValue()  const noexcept { return target_; }
private:
    float currentValue_ = 1e-9f, target_ = 1e-9f, step_ = 1.0f;
    int stepsToTarget_ = 0, countdown_ = 0;
};

} // namespace dubwize
