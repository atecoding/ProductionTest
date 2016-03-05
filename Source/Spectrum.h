#pragma once

class SpectrumWindow;

class Spectrum
{
public:
    Spectrum(int const numBins_, double const sampleRate_);
    ~Spectrum();
    
    void clear();
    void calculate(FFT *fft, SpectrumWindow *window, float const *source);
    void calculate(FFT *fft, SpectrumWindow *window, AudioSampleBuffer &source, int const channel, int const startPosition);
    void dump() const;
    void binsToDecibels(Array<float> &decibelArray);
    
    double calculateTHD(double const fundamentalFrequency, int const firstHarmonic, int const lastHarmonic);
    
    int getBinForFrequency(double const frequency) const;
    int getBinForHarmonic(double const fundamentalFrequency, int const harmonicOrder) const;
    double interpolatePeak(int const bin);
    
    int getNumBins() const
    {
        return numBins;
    }
    
    inline float getBin(int const bin) const
    {
        return buffer.getBin(bin);
    }
    
    void trimMirror();
    
private:
    int numBins;
    double sampleRate;
    
    inline void setBin(int const bin, float const value)
    {
        buffer.setBin(bin, value);
    }
    
    class Buffer
    {
    public:
        Buffer(int const numBins_) :
            length(numBins_ * 2 + EXTRA_LENGTH)
        {
            block.calloc(length);
        }
        
        void clear()
        {
            block.clear(length);
        }
        
        float * getFFTBuffer() const
        {
            return block.getData() + EXTRA_LENGTH;
        }
        inline void setBin(int const bin, float const value)
        {
            block[bin + EXTRA_LENGTH] = value;
        }
        inline float getBin(int const bin) const
        {
            return block[bin + EXTRA_LENGTH];
        }
        
    private:
        enum
        {
            EXTRA_LENGTH = 2
        };
        int length;
        HeapBlock<float> block;
    } buffer;
    
    friend class SpectrumAnalyzer;
};
