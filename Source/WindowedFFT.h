#pragma once

class SpectrumWindow
{
public:
    SpectrumWindow(int const windowLength_);
    
    int getLength() const
    {
        return windowLength;
    }
    float const * const getValues() const;
    
    void dump();
    
protected:
    HeapBlock<float> window;
    int windowLength;
};

class BlackmanHarrisWindow : public SpectrumWindow
{
public:
    BlackmanHarrisWindow(int const windowLength_, int const terms);
    
    static void test();

private:
    //
    // Constants for 7-term window
    //
    static const float a0;
    static const float a1;
    static const float a2;
    static const float a3;
    static const float a4;
    static const float a5;
    static const float a6;
};

class WindowedFFT
{
public:
    
    static void test(AudioBuffer<float>& buffer);
    
private:
    
};