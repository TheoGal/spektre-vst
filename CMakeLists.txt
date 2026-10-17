cmake_minimum_required(VERSION 3.22)
project(Spektre VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    # static runtime: the .vst3 works without installing the VC++ redistributable
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE: use a local checkout with -DJUCE_DIR=/path/to/JUCE, otherwise download it
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.8
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(Spektre
    PRODUCT_NAME                "Spektre"
    COMPANY_NAME                "TheoGal"
    BUNDLE_ID                   "gr.cnnbl.spektre"
    PLUGIN_MANUFACTURER_CODE    Thgl
    PLUGIN_CODE                 Spkt
    FORMATS                     VST3 AU Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Analyzer
    ICON_BIG                    Resources/icon.png
    # macOS: an app must say why it wants audio input, or the system blocks it
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Spektre analyzes the audio from your input device to show its spectrum. Nothing is recorded or sent anywhere."
    COPY_PLUGIN_AFTER_BUILD     FALSE)

juce_add_binary_data(SpektreData SOURCES
    Resources/fonts/BarlowCondensed-SemiBold.ttf
    Resources/fonts/BarlowCondensed-Bold.ttf
    Resources/fonts/JetBrainsMono-Regular.ttf
    Resources/fonts/JetBrainsMono-SemiBold.ttf)

target_sources(Spektre PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Analyzer.cpp
    Source/SpectrumView.cpp
    Source/SpektreLookAndFeel.cpp)

target_compile_definitions(Spektre PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Spektre
    PRIVATE
        SpektreData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Optional test harness (calibration checks + a rendered screenshot of the UI)
option(SPEKTRE_BUILD_TESTS "Build the test harness" OFF)
if(SPEKTRE_BUILD_TESTS)
    juce_add_console_app(SpektreTests PRODUCT_NAME "SpektreTests")
    target_sources(SpektreTests PRIVATE tests/Tests.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/Analyzer.cpp
        Source/SpectrumView.cpp Source/SpektreLookAndFeel.cpp)
    target_compile_definitions(SpektreTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="Spektre" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(SpektreTests PRIVATE SpektreData juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
