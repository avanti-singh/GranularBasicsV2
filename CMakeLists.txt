# Minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Set project name and version
project(Grawr VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------
# 1) Include the JUCE library
# ------------------------------------------------------------------
add_subdirectory("../JUCE" "${CMAKE_BINARY_DIR}/JUCE")

# ------------------------------------------------------------------
# 2) Include the react-juce module
#    (Adjust path if react-juce is stored elsewhere)
# ------------------------------------------------------------------
# add_subdirectory("../../react-juce" "${CMAKE_BINARY_DIR}/react-juce")

# ------------------------------------------------------------------
# 3) Add JUCE plugin target
# ------------------------------------------------------------------
juce_add_plugin(Grawr
    COMPANY_NAME "EP-353"
    PLUGIN_NAME "Grawr"

    # Formats you want to build:
    FORMATS AU VST3 Standalone

    # Plugin characteristics:
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSEi
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Source files for your plugin:
    SOURCES
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/PluginProcessor.h
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/PluginProcessor.cpp
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/PluginEditor.h
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/PluginEditor.cpp
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/RotaryKnob.h
        /Users/apple/Desktop/Spring25/Gransynth/backend/Grawr/Source/EQVisualizer.h
)

# ------------------------------------------------------------------
# 4) (Optional) macOS architecture
#    If you only want x86_64, keep this. Otherwise remove or modify.
# ------------------------------------------------------------------
# set(CMAKE_OSX_ARCHITECTURES "x86_64")

# ------------------------------------------------------------------
# 5) Link JUCE modules and react-juce
# ------------------------------------------------------------------
target_link_libraries(Grawr
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
        juce::juce_osc

        # React-juce:
        #juce::react_juce
)

# ------------------------------------------------------------------
# 6) Generate JUCE header
# ------------------------------------------------------------------
juce_generate_juce_header(Grawr)

# ------------------------------------------------------------------
# 7) (Optional) Define macros (e.g., enable WebBrowser for react-juce)
# ------------------------------------------------------------------
target_compile_definitions(Grawr
    PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_PLUGINHOST_VST=0
        JUCE_VST2_SDK_ENABLED=0
)
