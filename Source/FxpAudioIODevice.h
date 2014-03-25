/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-6 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef __FXPAUDIOIODEVICE_HEADER__
#define __FXPAUDIOIODEVICE_HEADER__

#include "AsioCallback.h"


//==============================================================================
/**
    Base class for an audio device with synchoronised input and output channels.

    Subclasses of this are used to implement different protocols such as DirectSound,
    ASIO, CoreAudio, etc.

    To create one of these, you'll need to use the AudioIODeviceType class - see the
    documentation for that class for more info.

    For an easier way of managing audio devices and their settings, have a look at the
    AudioDeviceManager class.

    @see AudioIODeviceType, AudioDeviceManager
*/
class FxpAudioIODevice
{
public:
    /** Destructor. */
    virtual ~FxpAudioIODevice();

    //==============================================================================
    /** Returns the device's name, (as set in the constructor). */
    const String& getName() const throw()                           { return name; }

    /** Returns the names of the available output channels on this device. */
    virtual const StringArray getOutputChannelNames() = 0;

    /** Returns the names of the available input channels on this device. */
    virtual const StringArray getInputChannelNames() = 0;

    /** Returns the number of sample-rates this device supports.

        To find out which rates are available on this device, use this method to
        find out how many there are, and getSampleRate() to get the rates.

        @see getSampleRate
    */
    virtual int getNumSampleRates() = 0;

    /** Returns one of the sample-rates this device supports.

        To find out which rates are available on this device, use getNumSampleRates() to
        find out how many there are, and getSampleRate() to get the individual rates.

        The sample rate is set by the open() method.

        (Note that for DirectSound some rates might not work, depending on combinations
        of i/o channels that are being opened).

        @see getNumSampleRates
    */
    virtual double getSampleRate (int index) = 0;

    /** Returns the number of sizes of buffer that are available.

        @see getBufferSizeSamples, getDefaultBufferSize
    */
    virtual int getNumBufferSizesAvailable() = 0;

    /** Returns one of the possible buffer-sizes.

        @param index    the index of the buffer-size to use, from 0 to getNumBufferSizesAvailable() - 1
        @returns a number of samples
        @see getNumBufferSizesAvailable, getDefaultBufferSize
    */
    virtual int getBufferSizeSamples (int index) = 0;

    /** Returns the default buffer-size to use.

        @returns a number of samples
        @see getNumBufferSizesAvailable, getBufferSizeSamples
    */
    virtual int getDefaultBufferSize() = 0;

    //==============================================================================
    /** Tries to open the device ready to play.

        @param inputChannels        a BitArray in which a set bit indicates that the corresponding
                                    input channel should be enabled
        @param outputChannels       a BitArray in which a set bit indicates that the corresponding
                                    output channel should be enabled
        @param sampleRate           the sample rate to try to use - to find out which rates are
                                    available, see getNumSampleRates() and getSampleRate()
        @param bufferSizeSamples    the size of i/o buffer to use - to find out the available buffer
                                    sizes, see getNumBufferSizesAvailable() and getBufferSizeSamples()
        @returns    an error description if there's a problem, or an empty string if it succeeds in
                    opening the device
        @see close
    */
    virtual const String open (const BitArray& inputChannels,
                               const BitArray& outputChannels,
                               double sampleRate,
                               int bufferSizeSamples) = 0;

    /** Closes and releases the device if it's open. */
    virtual void close() = 0;

    /** Returns true if the device is still open.

        A device might spontaneously close itself if something goes wrong, so this checks if
        it's still open.
    */
    virtual bool isOpen() = 0;

    /** Starts the device actually playing.

        This must be called after the device has been opened.

        @param callback     the callback to use for streaming the data.
        @see FxpAudioIODeviceCallback, open
    */
    virtual void start (FxpAudioIODeviceCallback* callback) = 0;

    /** Stops the device playing.

        Once a device has been started, this will stop it. Any pending calls to the
        callback class will be flushed before this method returns.
    */
    virtual void stop() = 0;

    /** Returns true if the device is still calling back.

        The device might mysteriously stop, so this checks whether it's
        still playing.
    */
    virtual bool isPlaying() = 0;

    /** Returns the last error that happened if anything went wrong. */
    virtual const String getLastError() = 0;

    //==============================================================================
    /** Returns the buffer size that the device is currently using.

        If the device isn't actually open, this value doesn't really mean much.
    */
    virtual int getCurrentBufferSizeSamples() = 0;

    /** Returns the sample rate that the device is currently using.

        If the device isn't actually open, this value doesn't really mean much.
    */
    virtual double getCurrentSampleRate() = 0;

    /** Returns the device's current physical bit-depth.

        If the device isn't actually open, this value doesn't really mean much.
    */
    virtual int getCurrentBitDepth() = 0;

    /** Returns the device's output latency.

        This is the delay in samples between a callback getting a block of data, and
        that data actually getting played.
    */
    virtual int getOutputLatencyInSamples() = 0;

    /** Returns the device's input latency.

        This is the delay in samples between some audio actually arriving at the soundcard,
        and the callback getting passed this block of data.
    */
    virtual int getInputLatencyInSamples() = 0;

    //==============================================================================
    /** True if this device can show a pop-up control panel for editing its settings.

        This is generally just true of ASIO devices. If true, you can call showControlPanel()
        to display it.
    */
    virtual bool hasControlPanel() const;

    /** Shows a device-specific control panel if there is one.

        This should only be called for devices which return true from hasControlPanel().
    */
    virtual bool showControlPanel();

	AsioCallback callbackObject;

    //==============================================================================
protected:
    /** Creates a device, setting its name member variable.
    */
    FxpAudioIODevice (const String& deviceName);

    /** @internal */
    String name;
};


#endif   // __FXPAUDIOIODEVICE_HEADER__
