#pragma once

namespace r8b
{
    class CDSPResampler24;
}

class Upsampler
{
public:
    Upsampler(double inputSampleRate_, double outputSampleRate_);
    
    int upsample(double* inputBuffer, double* outputBuffer, int inputSampleCount, int maxOutputSampleCount);
    
    static void test();
    
protected:
    double inputSampleRate, outputSampleRate;
    ScopedPointer<r8b::CDSPResampler24> resampler;
    
    enum
    {
        MAX_RESAMPLER_INPUT_SAMPLES = 1024
    };
};