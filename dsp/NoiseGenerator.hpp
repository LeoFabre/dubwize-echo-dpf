#pragma once
#include <vector>
#include <algorithm>
#include <cstdint>

namespace dubwize {

// ---------------------------------------------------------------------------
// Deterministic LCG replacing juce::Random
// ---------------------------------------------------------------------------
struct Rng {
    uint32_t s_ = 0x12345678u;
    float nextFloat() noexcept { s_ = s_ * 1664525u + 1013904223u; return (s_ >> 8) * (1.0f / 16777216.0f); }
    float uniformRandomValue() noexcept { return 2.0f * nextFloat() - 1.0f; }
};

// ---------------------------------------------------------------------------
// Base class
// ---------------------------------------------------------------------------
class NoiseGenerator {
public:
    virtual ~NoiseGenerator() = default;
    virtual void reset(float sampleRate) { fs_ = sampleRate; }
    virtual float nextValue() { return 0.0f; }
protected:
    float uniformRandomValue() { return rng_.uniformRandomValue(); }
    float fs_ = 44100.0f;
    Rng   rng_;
};

// ---------------------------------------------------------------------------
// White noise
// ---------------------------------------------------------------------------
class WhiteNoiseGenerator : public NoiseGenerator {
public:
    void reset(float sampleRate) override { NoiseGenerator::reset(sampleRate); }
    float nextValue() override { return uniformRandomValue(); }
};

// ---------------------------------------------------------------------------
// Pink noise  (Paul Kellett / musicdsp.org method)
// ---------------------------------------------------------------------------
class PinkNoiseGenerator : public NoiseGenerator {
public:
    void reset(float sampleRate) override {
        NoiseGenerator::reset(sampleRate);
        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;
    }

    float nextValue() override {
        float white = uniformRandomValue();
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f; // scale to ~[-1, 1]
    }

private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f,
          b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
};

// ---------------------------------------------------------------------------
// Brownian noise  (leaky integration + min/max normalisation)
// ---------------------------------------------------------------------------
class BrownianNoiseGenerator : public NoiseGenerator {
public:
    static constexpr int NUM_BUFFERED_SAMPLES = 44100;

    BrownianNoiseGenerator() {
        normalisedSamples.resize(NUM_BUFFERED_SAMPLES, 0.0f);
        raw.resize(NUM_BUFFERED_SAMPLES, 0.0f);   // pre-allocated; regeneration is alloc-free (RT-safe)
    }

    void reset(float sampleRate) override {
        NoiseGenerator::reset(sampleRate);
        lastUnnormalisedSample = 0.0f;
        generateNormalisedSamples();
        currPosition = 0;
    }

    float nextValue() override {
        if (currPosition >= NUM_BUFFERED_SAMPLES) {
            generateNormalisedSamples();
            currPosition = 0;
        }
        return normalisedSamples[currPosition++];
    }

private:
    void generateNormalisedSamples() {
        // Generate unnormalised brownian (leaky integration) into the pre-allocated scratch.
        for (int i = 0; i < NUM_BUFFERED_SAMPLES; ++i) {
            lastUnnormalisedSample = lastUnnormalisedSample * 0.95f + this->uniformRandomValue() * 0.05f;
            raw[i] = lastUnnormalisedSample;
        }

        // Min/max normalise to ±0.8
        float maxVal = *std::max_element(raw.begin(), raw.end());
        float minVal = *std::min_element(raw.begin(), raw.end());
        float range  = maxVal - minVal;
        if (range < 1e-9f) range = 1e-9f;

        for (int i = 0; i < NUM_BUFFERED_SAMPLES; ++i) {
            normalisedSamples[i] = ((raw[i] - minVal) / range * 2.0f - 1.0f) * 0.8f;
        }
    }

    std::vector<float> normalisedSamples;
    std::vector<float> raw;                    // pre-allocated scratch (avoids per-regen heap alloc)
    float lastUnnormalisedSample = 0.0f;
    int   currPosition           = 0;
};

} // namespace dubwize
