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

# The @2x UI images and the skyline window order, compiled into the binary as BinaryData.
juce_add_binary_data(BigCityAssets
    SOURCES
        assets/panel.png
        assets/knob_base.png
        assets/knob_rotor.png
        assets/screen_overlay.png
        assets/skyline_colour.png
        assets/skyline_grey.png
        assets/switch_fx_on.png
        assets/switch_bypassed.png
        assets/skyline-windows.json)

# Shared by the plugin and the tests, so the tests exercise the code that ships.
set(BIGCITY_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/CityKnob.cpp
    Source/BypassSwitch.cpp
    Source/KnobInput.cpp
    Source/KnobMotion.cpp
    Source/KnobPointer.cpp
    Source/SevenSegment.cpp
    Source/SevenSegmentReadout.cpp
    Source/Skyline.cpp
    Source/SkylineMorph.cpp
    Source/CityMacro.cpp
    Source/AirShelf.cpp
    Source/MicroShifter.cpp
    Source/WetFilter.cpp
    Source/AutoGain.cpp)

# Developer-only: exposes sound-tuning parameters in the generic editor. Never ship a build with this ON.
option(BIGCITY_TUNING "Developer-only sound tuning parameters; never ship" OFF)

if (BIGCITY_TUNING)
    message(WARNING "BIGCITY_TUNING is ON: this is a developer tuning build. Never ship or release it.")
    list(APPEND BIGCITY_SOURCES Source/TuningParameters.cpp)
endif()

target_sources(BigCity
    PRIVATE
        ${BIGCITY_SOURCES})

target_compile_definitions(BigCity
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        BIGCITY_TUNING=$<BOOL:${BIGCITY_TUNING}>)

target_link_libraries(BigCity
    PRIVATE
        BigCityAssets
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
        Tests/CityMacroTests.cpp
        Tests/KnobInputTests.cpp
        Tests/KnobMotionTests.cpp
        Tests/KnobPointerTests.cpp
        Tests/CityKnobTests.cpp
        Tests/BypassSwitchTests.cpp
        Tests/SevenSegmentTests.cpp
        Tests/SkylineMorphTests.cpp
        Tests/AirShelfTests.cpp
        Tests/MicroShifterTests.cpp
        Tests/WetFilterTests.cpp
        Tests/WetDriveTests.cpp
        Tests/AutoGainTests.cpp
        Tests/ProcessorTests.cpp)

target_include_directories(BigCityTests
    PRIVATE
        Source)

target_compile_definitions(BigCityTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="BigCity"
        BIGCITY_TUNING=$<BOOL:${BIGCITY_TUNING}>)

target_link_libraries(BigCityTests
    PRIVATE
        BigCityAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

add_test(NAME BigCityTests COMMAND BigCityTests)

# PRD §6 shelf response check. It reports findings rather than passing or failing, so it isn't a CTest test.
juce_add_console_app(ShelfCheck
    PRODUCT_NAME "ShelfCheck")

target_sources(ShelfCheck
    PRIVATE
        Source/AirShelf.cpp
        Source/CityMacro.cpp
        Tests/ShelfCheck.cpp)

target_include_directories(ShelfCheck
    PRIVATE
        Source)

target_compile_definitions(ShelfCheck
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(ShelfCheck
    PRIVATE
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# PRD §6 auto gain measurement: prints the loudness of each City Size against bypass and the curve
# that would match it. A tool for building SoundSpec::autoGainDb, so it isn't a CTest test.
juce_add_console_app(AutoGainMeasure
    PRODUCT_NAME "AutoGainMeasure")

target_sources(AutoGainMeasure
    PRIVATE
        ${BIGCITY_SOURCES}
        Tests/AutoGainMeasure.cpp)

target_include_directories(AutoGainMeasure
    PRIVATE
        Source)

target_compile_definitions(AutoGainMeasure
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="BigCity"
        BIGCITY_TUNING=$<BOOL:${BIGCITY_TUNING}>)

target_link_libraries(AutoGainMeasure
    PRIVATE
        BigCityAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
