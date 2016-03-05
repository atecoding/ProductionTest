#include "base.h"
#include "WindowedFFT.h"

//
// Constants for 7-term Blackman-Harris window
//
const float BlackmanHarrisWindow::a0 = 0.27105140069342f;
const float BlackmanHarrisWindow::a1 = 0.43329793923448f;
const float BlackmanHarrisWindow::a2 = 0.21812299954311f;
const float BlackmanHarrisWindow::a3 = 0.06592544638803f;
const float BlackmanHarrisWindow::a4 = 0.01081174209837f;
const float BlackmanHarrisWindow::a5 = 0.00077658482522f;
const float BlackmanHarrisWindow::a6 = 0.00001388721735f;

SpectrumWindow::SpectrumWindow(int const windowLength_)
{
    windowLength = windowLength_;
    window.malloc(windowLength);
}


float const * const SpectrumWindow::getValues() const
{
    return window;
}


void SpectrumWindow::dump()
{
    DBG("SpectrumWindow::dump");
    for (int i = 0; i < windowLength; ++i)
    {
        DBG("i:" << i << " " << window[i]);
    }
}

BlackmanHarrisWindow::BlackmanHarrisWindow(int const windowLength_, int const terms) :
SpectrumWindow(windowLength_)
{
    float sum = 0.0f;
    
    switch (terms)
    {
        case 4:
        {
            for (int i = 0; i < windowLength_; ++i)
            {
                float w = float_Pi * float(i) / float(windowLength_ - 1);
                
                window[i] = 0.35875f
                            - 0.48829f * cos(2.0f * w)
                            + 0.14128f * cos(4.0f * w)
                            - 0.01168f * cos(6.0f * w);
                sum += window[i];
            }
        }
        break;
            
        case 7:
        {
            for (int i = 0; i < windowLength_; ++i)
            {
                float w = 2.0f * float_Pi * float(i) / float(windowLength_);
                
                window[i] = a0
                            - a1 * cos(w)
                            + a2 * cos(2.0f * w)
                            - a3 * cos(3.0f * w)
                            + a4 * cos(4.0f * w)
                            - a5 * cos(5.0f * w)
                            + a6 * cos(6.0f * w);
                sum += window[i];
            }
        }
        break;
            
        default:
        {
            jassertfalse;
        }
        break;
    }
  
#if 0
    float scale = 1.0f;
    if (sum > 0.0f)
    {
        scale = 4.0f / (sum * sum);
    }
    
    float scaleInverse = 1.0f / scale;
    
    for (int i = 0; i < windowLength; ++i)
    {
        window[i] *= scaleInverse;
    }
#endif
    
    float scale = 2.0f / sum;
    for (int i = 0; i < windowLength_; ++i)
    {
        window[i] *= scale;
    }
}


void BlackmanHarrisWindow::test()
{
    BlackmanHarrisWindow window(1024, 4);
    float const * const windowValues = window.getValues();
    int windowLength = window.getLength();
    
    for (int i = 0; i < windowLength; ++i)
    {
        DBG("i:" << i << " " << windowValues[i]);
    }
}


void WindowedFFT::test(AudioBuffer<float>& buffer)
{
    int const order = 10;
    int const length = 1 << order;
    FFT fft(order, false);
    BlackmanHarrisWindow window(length, 4);
    AudioBuffer<float> tempBuffer(buffer.getNumChannels(), length);
    float const * const windowValues = window.getValues();
    
    float const *source = buffer.getReadPointer(0);
    float *dest = tempBuffer.getWritePointer(0);
    for (int i = 0; i < length; ++i)
    {
        dest[i] = source[i] * windowValues[i];
    }
    
    fft.performFrequencyOnlyForwardTransform(dest);
    
    for (int i = 0; i < length; ++i)
    {
        DBG("i:" << i << " " << dest[i]);
    }
}