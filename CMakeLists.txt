cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) -- must be set before project()
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
# Static MSVC runtime so users do not need to install redistributables
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(BlessedScratch VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local checkout if JUCE_DIR points to one, otherwise download it.
set(JUCE_DIR "" CACHE PATH "Optional path to a local JUCE checkout")
if(JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory(${JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BlessedScratch
    COMPANY_NAME                "Blessed"
    PLUGIN_MANUFACTURER_CODE    Blsd
    PLUGIN_CODE                 Bscr
    PRODUCT_NAME                "BLESSED SCRATCH"
    FORMATS                     VST3 Standalone
    VST3_CATEGORIES             Fx
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE)

juce_add_binary_data(BlessedAssets SOURCES
    Assets/logo.png
    Assets/fonts/Anton-Regular.ttf
    Assets/fonts/BarlowCondensed-Medium.ttf
    Assets/fonts/BarlowCondensed-SemiBold.ttf)

target_sources(BlessedScratch PRIVATE
    Source/ScratchEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(BlessedScratch PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(BlessedScratch
    PRIVATE
        BlessedAssets
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
