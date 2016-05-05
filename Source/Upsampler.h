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
	Result upsample(AudioBuffer<float> const &inputBuffer, int const inputChannel, AudioBuffer<float> &outputBuffer_, int const outputChannel, int &outputSampleCount_);
    
	void setOutputBufferSize(double seconds);

	HeapBlock<double> outputBuffer;
	int outputSampleCount;
    
    static void test();
    
    double getInputSampleRate() const
    {
        return inputSampleRate;
    }
    
    double getOutputSampleRate() const
    {
        return outputSampleRate;
    }
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