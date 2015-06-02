#pragma once

namespace r8b
{
    class CDSPResampler24;
}

class Upsampler
{
public:
	Upsampler(double inputSampleRate_, double outputSampleRate_);
	~Upsampler();
    
	void upsample(AudioSampleBuffer *inputBuffer);
	void setOutputBufferSize(double seconds);

	HeapBlock<double> outputBuffer;
	int outputSampleCount;
    
    static void test();
    
protected:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Upsampler)

    double inputSampleRate, outputSampleRate;
	HeapBlock<double> inputBlockBuffer;
	int maxOutputSampleCount;
	r8b::CDSPResampler24* resampler;
    
    enum
    {
        MAX_RESAMPLER_INPUT_SAMPLES = 1024
    };
};