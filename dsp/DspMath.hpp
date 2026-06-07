#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace dubwize {

inline constexpr float kMinGain = 1e-9f;
inline constexpr float kPi      = 3.14159265358979323846f;
inline constexpr float kTwoPi   = 6.28318530717958647692f;

inline float gainToDb(float x) noexcept { return 20.0f * std::log10(std::max(x, kMinGain)); }
inline float dbToGain(float db) noexcept { return std::pow(10.0f, 0.05f * db); }

template <typename T> inline T clamp(T x, T lo, T hi) noexcept { return std::max(lo, std::min(hi, x)); }
template <typename T> inline T jlimit(T lo, T hi, T x) noexcept { return std::max(lo, std::min(hi, x)); }
template <typename T> inline T jmax(T a, T b) noexcept { return a > b ? a : b; }
template <typename T> inline T jmin(T a, T b) noexcept { return a < b ? a : b; }

inline int nextPowerOfTwo(int n) noexcept {
    int p = 1; while (p < n) p <<= 1; return p;
}

// Bit-for-bit replica of juce::dsp::FastMathApproximations::tan (Padé approximant).
inline float fastTan(float x) noexcept {
    const float x2 = x * x;
    const float num = x * (-135135.0f + x2 * (17325.0f + x2 * (-378.0f + x2)));
    const float den = -135135.0f + x2 * (62370.0f + x2 * (-3150.0f + 28.0f * x2));
    return num / den;
}

// Bit-for-bit replica of juce::dsp::FastMathApproximations::sin (Padé approximant).
inline float fastSin(float x) noexcept {
    const float x2 = x * x;
    const float num = -x * (-11511339840.0f + x2 * (1640635920.0f + x2 * (-52785432.0f + x2 * 479249.0f)));
    const float den =       11511339840.0f + x2 * ( 277920720.0f + x2 * (   3177720.0f + x2 *  18361.0f));
    return num / den;
}

} // namespace dubwize
