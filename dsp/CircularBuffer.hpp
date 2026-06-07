#pragma once
#include <memory>
#include <cstring>
#include "DspMath.hpp"

namespace dubwize {

inline float cubicInterpolation(float y0, float y1, float y2, float y3, float fraction) {
    float fraction2 = fraction * fraction;
    float a0 = -0.5f*y0 + 1.5f*y1 - 1.5f*y2 + 0.5f*y3;
    float a1 =       y0 - 2.5f*y1 + 2.0f*y2 - 0.5f*y3;
    float a2 = -0.5f*y0           + 0.5f*y2;
    float a3 = y1;
    return a0*fraction*fraction2 + a1*fraction2 + a2*fraction + a3;
}

class CircularBuffer {
public:
    CircularBuffer() = default;
    void flushBuffer() { std::memset(buffer_.get(), 0, bufferLength_ * sizeof(float)); }
    void createCircularBuffer(unsigned int len) {
        writeIndex_ = 0;
        bufferLength_ = (unsigned) nextPowerOfTwo((int) len);
        wrapMask_ = bufferLength_ - 1;
        buffer_.reset(new float[bufferLength_]);
        flushBuffer();
    }
    void writeBuffer(float in) { buffer_[writeIndex_++] = in; writeIndex_ &= wrapMask_; }
    float readBuffer(int delaySamples) {
        int readIndex = (int) writeIndex_ - delaySamples;
        readIndex &= (int) wrapMask_;
        return buffer_[(unsigned) readIndex];
    }
    float readBuffer(float delayFractional, bool linear = false) {
        float frac = delayFractional - (int) delayFractional;
        float y1 = readBuffer((int) delayFractional);
        float y2 = readBuffer((int) delayFractional + 1);
        if (linear) return (1.0f - frac) * y1 + frac * y2;
        float y0 = readBuffer((int) delayFractional - 1);
        float y3 = readBuffer((int) delayFractional + 2);
        return cubicInterpolation(y0, y1, y2, y3, frac);
    }
    int getWriteIndex() const { return (int) writeIndex_; }
private:
    std::unique_ptr<float[]> buffer_;
    unsigned int writeIndex_ = 0, bufferLength_ = 1024, wrapMask_ = 1023;
};

} // namespace dubwize
