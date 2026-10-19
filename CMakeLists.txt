cmake_minimum_required(VERSION 3.22)
project(AndYouRevTrem VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(REVTREM_BUILD_TESTS "Build offline DSP test" OFF)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AndYouRevTrem
    COMPANY_NAME "AndYou"
    PLUGIN_MANUFACTURER_CODE Andy
    PLUGIN_CODE Rtrm
    FORMATS VST3 Standalone
    PRODUCT_NAME "AndYou RevTrem"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AndYouRevTrem PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h)

target_compile_definitions(AndYouRevTrem PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AndYouRevTrem
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (REVTREM_BUILD_TESTS)
    juce_add_console_app(DspTest PRODUCT_NAME "DspTest")
    target_sources(DspTest PRIVATE Tests/DspTest.cpp Source/PluginProcessor.cpp)
    target_compile_definitions(DspTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(DspTest PRIVATE Source)
    target_link_libraries(DspTest PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
