#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <new>
#endif

#pragma warning(disable:4100) // disable warning for parameter not used

#define  JUCE_DEFINE_T_MACRO 1
//#define  JUCE_FORCE_DEBUG
//#define  JUCE_LOG_ASSERTIONS
#define  JUCE_ASIO 1
//#define  JUCE_WASAPI
//#define  JUCE_DIRECTSOUND
//#define  JUCE_ALSA
//#define  JUCE_QUICKTIME
//#define  JUCE_OPENGL
//#define  JUCE_USE_FLAC
//#define  JUCE_USE_OGGVORBIS
#define  JUCE_USE_CDBURNER 0
//#define  JUCE_USE_CDREADER
//#define  JUCE_USE_CAMERA
//#define  JUCE_ENABLE_REPAINT_DEBUGGING
//#define  JUCE_USE_XINERAMA
//#define  JUCE_USE_XSHM
//#define  JUCE_USE_XRENDER
//#define  JUCE_USE_XCURSOR
//#define  JUCE_PLUGINHOST_VST
//#define  JUCE_PLUGINHOST_AU
//#define  JUCE_ONLY_BUILD_CORE_LIBRARY
//#define  JUCE_WEB_BROWSER
//#define  JUCE_SUPPORT_CARBON
#define  JUCE_CHECK_MEMORY_LEAKS 1
#define  JUCE_CATCH_UNHANDLED_EXCEPTIONS 0

#include "../JuceLibraryCode/JuceHeader.h"

#define WRITE_WAVE_FILES 1

enum
{
	MESSAGE_AUDIO_TEST_DONE,
	MESSAGE_MIDI_TEST_DONE,
    MESSAGE_AIOS_CALIBRATION_DONE
};