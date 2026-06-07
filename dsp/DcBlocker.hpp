#pragma once
namespace dubwize {
class DcBlocker {
public:
    float processSample(float x) { float y = x - xn1_ + coeff_ * yn1_; xn1_ = x; yn1_ = y; return y; }
    void reset(float sampleRate) { fs_ = sampleRate; xn1_ = 0.0f; yn1_ = 0.0f; }
private:
    static constexpr float coeff_ = 0.995f;
    float fs_ = 44100.0f, xn1_ = 0.0f, yn1_ = 0.0f;
};
} // namespace dubwize
