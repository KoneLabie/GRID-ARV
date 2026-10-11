cmake_minimum_required(VERSION 3.22)

# ---- Build options (must be set before project()) ---------------------------
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
# Static MSVC runtime: users do not need to install the VC++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(GridARV VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_VST2 "Also build VST2 (requires valid pre-October-2018 Steinberg written permission, private SDK + JUCE 7)" OFF)
set(VST2_SDK_PATH "" CACHE PATH "Folder that contains pluginterfaces/vst2.x/aeffect.h")
# JUCE 8 dropped the VST2 wrapper. For VST2 builds use:  -DJUCE_TAG=7.0.12
set(JUCE_TAG "8.0.4" CACHE STRING "JUCE git tag to build against")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${JUCE_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# ---- Plugin formats ---------------------------------------------------------
set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

if(ENABLE_VST2)
    if(NOT EXISTS "${VST2_SDK_PATH}/pluginterfaces/vst2.x/aeffect.h")
        message(FATAL_ERROR "ENABLE_VST2 is ON but VST2_SDK_PATH is wrong: '${VST2_SDK_PATH}'")
    endif()
    juce_set_vst2_sdk_path("${VST2_SDK_PATH}")
    list(APPEND PLUGIN_FORMATS VST)
    message(STATUS "VST2 build enabled")
endif()

set(VIS_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VisualRenderer.cpp)

set(VIS_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# ---- VST3 / AU / (VST2) plugin ---------------------------------------------
juce_add_plugin(GridARVPlugin
    COMPANY_NAME               "Kone Labie"
    PLUGIN_MANUFACTURER_CODE   Kone
    PLUGIN_CODE                Garv
    FORMATS                    ${PLUGIN_FORMATS}
    PRODUCT_NAME               "Grid ARV"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    VST3_CATEGORIES            Fx Analyzer
    AU_MAIN_TYPE               kAudioUnitType_Effect
    VST2_CATEGORY              kPlugCategEffect)

target_sources(GridARVPlugin PRIVATE ${VIS_SOURCES})
target_compile_definitions(GridARVPlugin PUBLIC ${VIS_DEFS})
target_link_libraries(GridARVPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Standalone app (own host: input device picker, fullscreen) -------------
juce_add_gui_app(GridARVStandalone
    COMPANY_NAME  "Kone Labie"
    PRODUCT_NAME  "Grid ARV"
    VERSION       ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Grid ARV needs audio input so it can react to sound.")

target_sources(GridARVStandalone PRIVATE ${VIS_SOURCES} Source/StandaloneApp.cpp)
target_compile_definitions(GridARVStandalone PRIVATE ${VIS_DEFS} VIS_STANDALONE_APP=1)
target_link_libraries(GridARVStandalone
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
