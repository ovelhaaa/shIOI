cmake_minimum_required(VERSION 3.15)
project(shIOI_Juce VERSION 1.0.0)

# Fetch JUCE automatically to make the repo plug-and-play
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0 # Or use latest stable tag
)
FetchContent_MakeAvailable(juce)

# Add core_dsp interface library
add_subdirectory(../core_dsp build_core_dsp)

# JUCE Plugin Configuration
juce_add_plugin(shIOI_Plugin
    COMPANY_NAME "ShIOI"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Sioi"
    PLUGIN_CODE "Sh10"
    FORMATS VST3 Standalone
)

# Link dependencies
target_link_libraries(shIOI_Plugin
    PRIVATE
        core_dsp
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Add source files
target_include_directories(shIOI_Plugin PRIVATE Source)
target_sources(shIOI_Plugin PRIVATE 
    Source/PluginProcessor.cpp 
    Source/PluginProcessor.h
)

# Fix missing VST2 SDK headers by disabling VST2 compatibility in VST3
target_compile_definitions(shIOI_Plugin PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
)
