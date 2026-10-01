cmake_minimum_required(VERSION 3.22)

# Set before project() so the build is universal and targets macOS 12 without command-line flags.
# 12.0 is the oldest target Xcode 27 supports.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)

project(BigCity VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(BigCity
    COMPANY_NAME "sunshower audio"
    PLUGIN_MANUFACTURER_CODE Snsh
    PLUGIN_CODE Bgct
    BUNDLE_ID com.sunshoweraudio.bigcity
    PRODUCT_NAME "BigCity"
    FORMATS VST3 Standalone
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(BigCity
    PRIVATE
        Source/PluginProcessor.cpp)

target_compile_definitions(BigCity
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BigCity
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
