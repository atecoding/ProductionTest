#include "base.h"
#include "FrequencySweepAudioSource.h"
#include "Upsampler.h"
#include "wavefile.h"

#undef T        // fix conflict with legacy JUCE macro
#include "r8brain/CDSPResampler.h"

void Upsampler::test()
{
    //
    // Make some audio data
    //
    int sampleRate = 48000;
    FrequencySweepAudioSource generator;
    AudioSampleBuffer sourceBuffer(1, sampleRate * 2);
    AudioSourceChannelInfo asi(&sourceBuffer, 0, sourceBuffer.getNumSamples());
    
    generator.prepareToPlay(sourceBuffer.getNumSamples(), sampleRate);
    generator.getNextAudioBlock(asi);
    generator.releaseResources();

    //
    // Create the upsampler
    //
    int const upsampleFactor = 4;
    Upsampler upsampler(sampleRate, sampleRate * upsampleFactor);
    HeapBlock<double> upsamplerInputBuffer, upsamplerOutputBuffer;
    int upsamplerOutputBufferSamples = sourceBuffer.getNumSamples() * upsampleFactor * 2;
    
    upsamplerInputBuffer.allocate(sourceBuffer.getNumSamples(), true);
    upsamplerOutputBuffer.allocate(upsamplerOutputBufferSamples, true);
    
    //
    // Convert data to doubles
    //
    const float* source = sourceBuffer.getReadPointer(0);
    for (int i = 0; i < sourceBuffer.getNumSamples(); ++i)
    {
        upsamplerInputBuffer[i] = source[i];
    }
    
    //
    // Upsample
    //
    int upsampledCount = upsampler.upsample( upsamplerInputBuffer,
                                            upsamplerOutputBuffer,
                                            sourceBuffer.getNumSamples(),
                                            upsamplerOutputBufferSamples);
    
    //
    // Convert upsampled data to float
    //
    AudioSampleBuffer finalBuffer(1, upsamplerOutputBufferSamples);
    
    float *destination = finalBuffer.getWritePointer(0);
    for (int i = 0; i < upsampledCount; ++i)
    {
        destination[i] = upsamplerOutputBuffer[i];
    }
    
    WriteWaveFile("upsample.wav", sampleRate * upsampleFactor, &finalBuffer, upsamplerOutputBufferSamples);
}


Upsampler::Upsampler(double inputSampleRate_, double outputSampleRate_) :
inputSampleRate(inputSampleRate_),
outputSampleRate(outputSampleRate_)
{
    resampler = new r8b::CDSPResampler24( inputSampleRate_, outputSampleRate_, MAX_RESAMPLER_INPUT_SAMPLES);
}

int Upsampler::upsample(double* inputBuffer, double* outputBuffer, int inputSampleCount, int maxOutputSampleCount)
{
    int outputSampleCount = 0;
    
    //DBG("upsample inputSampleCount:" << inputSampleCount);
    
    if (nullptr == resampler)
        return 0;
    
    while (inputSampleCount > 0)
    {
        double* outputBlock = nullptr;
        
        int inputBlockSampleCount = jmin( inputSampleCount, (int)MAX_RESAMPLER_INPUT_SAMPLES);
        int outputBlockSampleCount = resampler->process(inputBuffer, inputBlockSampleCount, outputBlock);
        
        int outputSpaceRemaining = maxOutputSampleCount - outputSampleCount;
        int outputCopyCount = jmin( outputSpaceRemaining, outputBlockSampleCount);
        
        for (int i = 0; i < outputCopyCount; ++i)
        {
            outputBuffer[i] = outputBlock[i];
        }
        
        inputSampleCount -= inputBlockSampleCount;
        inputBuffer += inputBlockSampleCount;
        outputSampleCount += outputCopyCount;
        outputBuffer += outputCopyCount;
    }
    
    //DBG("   outputSampleCount:" << outputSampleCount);
    
    return outputSampleCount;
}


