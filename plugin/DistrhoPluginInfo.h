#pragma once

#define DISTRHO_PLUGIN_BRAND       "Nexus"
#define DISTRHO_PLUGIN_NAME        "Dubwize"
#define DISTRHO_PLUGIN_URI         "https://github.com/LeoFabre/dubwize-echo-dpf"
#define DISTRHO_PLUGIN_CLAP_ID     "fr.nexus.dubwize"
#define DISTRHO_PLUGIN_NUM_INPUTS  2
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2
#define DISTRHO_PLUGIN_IS_RT_SAFE  1
#ifndef DISTRHO_PLUGIN_HAS_UI
  #define DISTRHO_PLUGIN_HAS_UI    0   /* CMake overrides this when UI is built */
#endif
#define DISTRHO_PLUGIN_WANT_PROGRAMS   0
#define DISTRHO_PLUGIN_WANT_STATE      0
#define DISTRHO_PLUGIN_WANT_TIMEPOS    1   /* host BPM for host-sync */
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 0   /* tap button arrives as a CC→param mapping in Sushi */

#define DISTRHO_UI_USE_NANOVG     1
