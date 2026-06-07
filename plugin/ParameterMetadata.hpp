#pragma once
#include <cstdint>

namespace dubwize {

enum class Param : uint32_t {
    // Control params (45)
    mix,
    time,
    feedback,
    toneType,
    tapeDrive,
    tapeComp,
    tapeAge,
    tapeBias,
    effectsRouting,
    beatMultiply,
    pingPong,
    crossFeed,
    hold,
    time2,
    timeLink,
    delayEnabled,
    tapTempoEnabled,
    tapTempoButton,
    hostSyncEnabled,
    modRate,
    modDepth,
    modWave,
    modEnabled,
    noiseLevel,
    noiseType,
    noiseDucking,
    noiseEnabled,
    flipPhase,
    bcDepth,
    decimReduction,
    decimStereoSpread,
    decimEnabled,
    hpfCutoff,
    hpfQ,
    hpfPosition,
    lpfCutoff,
    lpfQ,
    lpfPosition,
    filterGainComp,
    filterSoftClip,
    filterEnabled,
    bmLevel,
    bmOperation,
    bmOperands,
    bmEnabled,
    // Output params (3)
    outTapInterval,
    outTapTimeSnapshot,
    outHostSyncInterval,
};

constexpr int kNumControlParams = 45;
constexpr int kNumOutputParams  = 3;

struct ParamInfo {
    const char* symbol;
    const char* name;
    float min;
    float max;
    float def;
    bool isBool;
    bool isInteger;
    bool isOutput;
    bool isLogarithmic;
    int numChoices;
    const char* const* choices;
};

// ---- Choice string arrays --------------------------------------------------

static const char* kChoicesToneType[]      = { "DIGITAL", "TAPE" };
static const char* kChoicesEffectsRouting[]= { "LOOP", "WET", "INPUT" };
static const char* kChoicesBeatMultiply[]  = { "0.25", "0.3333333", "0.5", "0.6666666", "0.75",
                                                "1.0", "1.25", "1.333333", "1.5" };
static const char* kChoicesModWave[]       = { "SIN", "TRI" };
static const char* kChoicesNoiseType[]     = { "WHITE", "BROWNIAN", "PINK" };
static const char* kChoicesFilterPos[]     = { "PRE BITMOD", "POST BITMOD", "FEEDBACK" };
static const char* kChoicesBmOperation[]   = { "NONE", "XOR", "AND", "OR" };
static const char* kChoicesBmOperands[]    = { "POST FX + POST FX", "PRE FX + POST FX", "DRY + POST FX" };

// ---- Parameter table -------------------------------------------------------
// Ordering must exactly match enum class Param above.
// Float params:   isBool=false, isInteger=false, numChoices=0, choices=nullptr
// Bool params:    isBool=true,  isInteger=false, min=0, max=1
// Choice params:  isBool=false, isInteger=true,  min=0, max=numChoices-1
// Output params:  isOutput=true (always float)
// isLogarithmic=true for: time, time2, tapeDrive, bcDepth, decimReduction,
//                         hpfCutoff, lpfCutoff

static constexpr ParamInfo kParams[48] = {
    // 0  mix
    { "mix",               "Mix",               0.0f,   100.0f,    50.0f,  false, false, false, false, 0, nullptr },
    // 1  time
    { "time",              "Time",              1.0f,  2000.0f,   200.0f,  false, false, false, true,  0, nullptr },
    // 2  feedback
    { "feedback",          "Feedback",          0.0f,   100.0f,    65.0f,  false, false, false, false, 0, nullptr },
    // 3  toneType
    { "toneType",          "Type",              0.0f,     1.0f,     0.0f,  false, true,  false, false, 2, kChoicesToneType },
    // 4  tapeDrive
    { "tapeDrive",         "Tape Drive",        0.0f,    12.0f,     0.0f,  false, false, false, true,  0, nullptr },
    // 5  tapeComp
    { "tapeComp",          "Tape Comp",         0.0f,   100.0f,     0.0f,  false, false, false, false, 0, nullptr },
    // 6  tapeAge
    { "tapeAge",           "Tape Age",          0.0f,   100.0f,    50.0f,  false, false, false, false, 0, nullptr },
    // 7  tapeBias
    { "tapeBias",          "Tape Bias",        -0.5f,    0.5f,     0.0f,  false, false, false, false, 0, nullptr },
    // 8  effectsRouting
    { "effectsRouting",    "Effects Routing",   0.0f,     2.0f,     1.0f,  false, true,  false, false, 3, kChoicesEffectsRouting },
    // 9  beatMultiply
    { "beatMultiply",      "Beat Multiply",     0.0f,     8.0f,     5.0f,  false, true,  false, false, 9, kChoicesBeatMultiply },
    // 10 pingPong
    { "pingPong",          "Ping Pong",         0.0f,   100.0f,     0.0f,  false, false, false, false, 0, nullptr },
    // 11 crossFeed
    { "crossFeed",         "Cross Feed",        0.0f,   100.0f,     0.0f,  false, false, false, false, 0, nullptr },
    // 12 hold
    { "hold",              "Hold",              0.0f,     1.0f,     0.0f,  true,  false, false, false, 0, nullptr },
    // 13 time2
    { "time2",             "Time 2",            1.0f,  2000.0f,   100.0f,  false, false, false, true,  0, nullptr },
    // 14 timeLink
    { "timeLink",          "Time Link",         0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 15 delayEnabled
    { "delayEnabled",      "Delay Enabled",     0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 16 tapTempoEnabled
    { "tapTempoEnabled",   "Tap Tempo Enabled", 0.0f,     1.0f,     0.0f,  true,  false, false, false, 0, nullptr },
    // 17 tapTempoButton
    { "tapTempoButton",    "Tap Tempo Button",  0.0f,     1.0f,     0.0f,  true,  false, false, false, 0, nullptr },
    // 18 hostSyncEnabled
    { "hostSyncEnabled",   "Host Sync",         0.0f,     1.0f,     0.0f,  true,  false, false, false, 0, nullptr },
    // 19 modRate
    { "modRate",           "Mod Rate",          0.02f,   10.0f,    0.02f,  false, false, false, false, 0, nullptr },
    // 20 modDepth
    { "modDepth",          "Mod Depth",         0.0f,   100.0f,     0.0f,  false, false, false, false, 0, nullptr },
    // 21 modWave
    { "modWave",           "Mod Wave",          0.0f,     1.0f,     1.0f,  false, true,  false, false, 2, kChoicesModWave },
    // 22 modEnabled
    { "modEnabled",        "Time Mod Enabled",  0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 23 noiseLevel
    { "noiseLevel",        "Noise Level",     -60.0f,     0.0f,   -60.0f,  false, false, false, false, 0, nullptr },
    // 24 noiseType
    { "noiseType",         "Noise Type",        0.0f,     2.0f,     0.0f,  false, true,  false, false, 3, kChoicesNoiseType },
    // 25 noiseDucking
    { "noiseDucking",      "Noise Ducking",     0.0f,   100.0f,    50.0f,  false, false, false, false, 0, nullptr },
    // 26 noiseEnabled
    { "noiseEnabled",      "Noise Enabled",     0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 27 flipPhase
    { "flipPhase",         "Flip Phase",        0.0f,     1.0f,     0.0f,  true,  false, false, false, 0, nullptr },
    // 28 bcDepth
    { "bcDepth",           "Bit Crush",         0.0f,     2.0f,    29.8e-9f, false, false, false, true, 0, nullptr },
    // 29 decimReduction
    { "decimReduction",    "Sample Rate",    0.0001f,     1.0f,     1.0f,  false, false, false, true,  0, nullptr },
    // 30 decimStereoSpread
    { "decimStereoSpread", "Stereo Spread",     0.0f,     0.5f,     0.0f,  false, false, false, false, 0, nullptr },
    // 31 decimEnabled
    { "decimEnabled",      "Decimation Enabled",0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 32 hpfCutoff
    { "hpfCutoff",         "LowCut Freq",      20.0f, 20480.0f,    20.0f,  false, false, false, true,  0, nullptr },
    // 33 hpfQ
    { "hpfQ",              "LowCut Res",        0.0f,     2.0f,     1.0f,  false, false, false, false, 0, nullptr },
    // 34 hpfPosition
    { "hpfPosition",       "LowCut Position",   0.0f,     2.0f,     0.0f,  false, true,  false, false, 3, kChoicesFilterPos },
    // 35 lpfCutoff
    { "lpfCutoff",         "HighCut Freq",     20.0f, 20480.0f, 20480.0f,  false, false, false, true,  0, nullptr },
    // 36 lpfQ
    { "lpfQ",              "HighCut Res",       0.0f,     2.0f,     1.0f,  false, false, false, false, 0, nullptr },
    // 37 lpfPosition
    { "lpfPosition",       "HighCut Position",  0.0f,     2.0f,     0.0f,  false, true,  false, false, 3, kChoicesFilterPos },
    // 38 filterGainComp
    { "filterGainComp",    "Filter Gain Comp",  0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 39 filterSoftClip
    { "filterSoftClip",    "Filter Soft Clip",  0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 40 filterEnabled
    { "filterEnabled",     "Filtering Enabled", 0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // 41 bmLevel
    { "bmLevel",           "BitMod Level",    -60.0f,     0.0f,   -60.0f,  false, false, false, false, 0, nullptr },
    // 42 bmOperation
    { "bmOperation",       "BitMod Operation",  0.0f,     3.0f,     0.0f,  false, true,  false, false, 4, kChoicesBmOperation },
    // 43 bmOperands
    { "bmOperands",        "BitMod Operands",   0.0f,     2.0f,     0.0f,  false, true,  false, false, 3, kChoicesBmOperands },
    // 44 bmEnabled
    { "bmEnabled",         "BitMod Enabled",    0.0f,     1.0f,     1.0f,  true,  false, false, false, 0, nullptr },
    // --- Output params (3) ---
    // 45 outTapInterval
    { "tapInterval",       "Tap Interval",      1.0f,  2000.0f,   500.0f,  false, false, true,  false, 0, nullptr },
    // 46 outTapTimeSnapshot
    { "tapTimeSnapshot",   "Tap Time Snapshot", 1.0f,  2000.0f,   100.0f,  false, false, true,  false, 0, nullptr },
    // 47 outHostSyncInterval
    { "hostSyncInterval",  "Host Sync Interval",1.0f,  2000.0f,   500.0f,  false, false, true,  false, 0, nullptr },
};

inline const ParamInfo& paramInfo(Param p) {
    return kParams[static_cast<uint32_t>(p)];
}

} // namespace dubwize
