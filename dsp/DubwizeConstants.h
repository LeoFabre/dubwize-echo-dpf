// Derived from StrangeReturns by JackWithOneEye
// Original: https://github.com/JackWithOneEye/StrangeReturns
// Licensed under the Apache License, Version 2.0
// Modified by Leo Fabre / Dubplex
#pragma once

constexpr int MAX_DELAY_TIME_SEC = 2;
constexpr float MIN_DELAY_TIME_MS = 1.0f;

constexpr float MIN_MOD_RATE_HZ = 0.02f;
constexpr float MAX_MOD_RATE_HZ = 10.0f;

constexpr float MIN_GAIN_DB = -60.0f;
constexpr float MAX_GAIN_DB = 0.0f;

constexpr float MIN_NOISE_LEVEL_DB = -60.0f;
constexpr float MAX_NOISE_LEVEL_DB = 0.0f;
constexpr float MIN_NOISE_DUCKING_PCT = 0.0f;
constexpr float MAX_NOISE_DUCKING_PCT = 100.0f;

constexpr float MIN_BITCRUSHER_Q = 29.8e-9f;
constexpr float MAX_BITCRUSHER_Q = 2.0f;

constexpr float MIN_DECIMATOR_RATIO = 0.0001f;
constexpr float MAX_DECIMATOR_RATIO = 1.0f;

constexpr float MIN_FILTER_CUTOFF_FREQ = 20.0f;
constexpr float MAX_FILTER_CUTOFF_FREQ = 20480.0f;

constexpr float MIN_FILTER_Q = 0.707f;
constexpr float MAX_FILTER_Q = 5.0f;

constexpr int NUM_CHANNELS = 2;

// Smoothed-value ramp length (was ProcessorUtils.h::SMOOTHED_VAL_RAMP_LEN_SEC)
constexpr float SMOOTHED_VAL_RAMP_LEN_SEC = 0.1f;

// Parameter defaults
constexpr float DEFAULT_MIX_PCT = 50.0f;
constexpr float DEFAULT_TIME_MS = 200.0f;
constexpr float DEFAULT_FEEDBACK_PCT = 65.0f;
constexpr int DEFAULT_TONE_TYPE_INDEX = 0;
constexpr float MIN_TAPE_DRIVE = 0.0f;
constexpr float MAX_TAPE_DRIVE = 12.0f;
constexpr float DEFAULT_TAPE_DRIVE = 0.0f;
constexpr float MIN_TAPE_COMP_PCT = 0.0f;
constexpr float MAX_TAPE_COMP_PCT = 100.0f;
constexpr float DEFAULT_TAPE_COMP_PCT = 0.0f;
constexpr float MIN_TAPE_AGE_PCT = 0.0f;
constexpr float MAX_TAPE_AGE_PCT = 100.0f;
constexpr float DEFAULT_TAPE_AGE_PCT = 50.0f;
constexpr float MIN_TAPE_BIAS = -0.5f;
constexpr float MAX_TAPE_BIAS = 0.5f;
constexpr float DEFAULT_TAPE_BIAS = 0.0f;
constexpr int DEFAULT_EFFECTS_ROUTING_INDEX = 1;
constexpr int DEFAULT_BEAT_MULTIPLY_INDEX = 5;
constexpr float DEFAULT_PING_PONG_PCT = 0.0f;
constexpr float DEFAULT_CROSS_FEED_PCT = 0.0f;
constexpr bool DEFAULT_HOLD_ENABLED = false;
constexpr float DEFAULT_TIME2_MS = 100.0f;
constexpr bool DEFAULT_TIME_LINK_ENABLED = true;
constexpr bool DEFAULT_DELAY_ENABLED = true;

constexpr bool DEFAULT_TAP_TEMPO_ENABLED = false;
constexpr bool DEFAULT_TAP_TEMPO_BUTTON = false;
constexpr float DEFAULT_TAP_INTERVAL_MS = 500.0f;
constexpr float DEFAULT_TAP_TIME_SNAPSHOT_MS = 100.0f;
constexpr bool DEFAULT_HOST_SYNC_ENABLED = false;
constexpr float DEFAULT_HOST_SYNC_INTERVAL_MS = 500.0f;

constexpr float DEFAULT_MOD_RATE_HZ = MIN_MOD_RATE_HZ;
constexpr float DEFAULT_MOD_DEPTH_PCT = 0.0f;
constexpr int DEFAULT_MOD_WAVE_INDEX = 1;
constexpr bool DEFAULT_MOD_ENABLED = true;

constexpr float DEFAULT_NOISE_LEVEL_DB = MIN_NOISE_LEVEL_DB;
constexpr int DEFAULT_NOISE_TYPE_INDEX = 0;
constexpr float DEFAULT_NOISE_DUCKING_PCT = 50.0f;
constexpr bool DEFAULT_NOISE_ENABLED = true;

constexpr bool DEFAULT_FLIP_PHASE = false;
constexpr float DEFAULT_BITCRUSHER_Q = MIN_BITCRUSHER_Q;

constexpr float DEFAULT_DECIMATOR_RATIO = MAX_DECIMATOR_RATIO;
constexpr float DEFAULT_DECIMATOR_STEREO_SPREAD = 0.0f;
constexpr bool DEFAULT_DECIMATOR_ENABLED = true;

constexpr float DEFAULT_HPF_CUTOFF_HZ = MIN_FILTER_CUTOFF_FREQ;
constexpr float DEFAULT_HPF_Q_UI = 1.0f;
constexpr float DEFAULT_HPF_Q_LIN = MIN_FILTER_Q;
constexpr int DEFAULT_HPF_POSITION_INDEX = 0;
constexpr float DEFAULT_LPF_CUTOFF_HZ = MAX_FILTER_CUTOFF_FREQ;
constexpr float DEFAULT_LPF_Q_UI = 1.0f;
constexpr float DEFAULT_LPF_Q_LIN = MIN_FILTER_Q;
constexpr int DEFAULT_LPF_POSITION_INDEX = 0;
constexpr bool DEFAULT_FILTER_GAIN_COMP = true;
constexpr bool DEFAULT_FILTER_SOFT_CLIP = true;
constexpr bool DEFAULT_FILTER_ENABLED = true;

constexpr float DEFAULT_BM_LEVEL_DB = MIN_GAIN_DB;
constexpr int DEFAULT_BM_OPERATION_INDEX = 0;
constexpr int DEFAULT_BM_OPERANDS_INDEX = 0;
constexpr bool DEFAULT_BM_ENABLED = true;

// DSP parameter fallbacks
constexpr float DEFAULT_DELAY_PARAM_FEEDBACK_PCT = 0.0f;
constexpr float DEFAULT_EFFECTS_PARAM_BC_DEPTH_LIN = 0.0f;

// Noise sidechain envelope
constexpr float NOISE_DUCKING_ATTACK_SEC = 0.005f;
constexpr float NOISE_DUCKING_RELEASE_SEC = 0.150f;
constexpr float NOISE_WITH_INPUT_RELEASE_SEC = 0.035f;
constexpr float NOISE_DUCKING_SIDECHAIN_GAIN = 4.0f;
constexpr float NOISE_DUCKING_ENVELOPE_CURVE = 0.5f;
