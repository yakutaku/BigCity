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

# Shared by the plugin and the tests, so the tests exercise the code that ships.
set(BIGCITY_SOURCES
    Source/PluginProcessor.cpp
    Source/CityMacro.cpp)

target_sources(BigCity
    PRIVATE
        ${BIGCITY_SOURCES})

target_compile_definitions(BigCity
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BigCity
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Unit tests: run with `ctest --test-dir build -C Release`.
enable_testing()

juce_add_console_app(BigCityTests
    PRODUCT_NAME "BigCityTests")

target_sources(BigCityTests
    PRIVATE
        ${BIGCITY_SOURCES}
        Tests/TestMain.cpp
        Tests/CityMacroTests.cpp)

target_include_directories(BigCityTests
    PRIVATE
        Source)

target_compile_definitions(BigCityTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="BigCity")

target_link_libraries(BigCityTests
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

add_test(NAME BigCityTests COMMAND BigCityTests)
