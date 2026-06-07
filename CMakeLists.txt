cmake_minimum_required(VERSION 3.22)
project(Dubwize VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUBWIZE_BUILD_UI    "Build the optional NanoVG UI" ON)
option(DUBWIZE_BUILD_TESTS "Build offline DSP tests"      ON)

add_subdirectory(dpf)

# The plugin target is only created once its DSP facade source exists. During the
# DSP-module development phase (before the facade is written) this lets the test
# tree configure and build standalone via `cmake -B build-host`.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin/DubwizePlugin.cpp")
    if(DUBWIZE_BUILD_UI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin/DubwizeUI.cpp")
        dpf_add_plugin(Dubwize
            TARGETS vst3 lv2
            UI_TYPE opengl
            FILES_DSP plugin/DubwizePlugin.cpp
            FILES_UI  plugin/DubwizeUI.cpp)
        target_compile_definitions(Dubwize PUBLIC DISTRHO_PLUGIN_HAS_UI=1)
    else()
        dpf_add_plugin(Dubwize
            TARGETS vst3 lv2
            FILES_DSP plugin/DubwizePlugin.cpp)
        target_compile_definitions(Dubwize PUBLIC DISTRHO_PLUGIN_HAS_UI=0)
    endif()

    target_include_directories(Dubwize PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/plugin
        ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
endif()

if(DUBWIZE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
