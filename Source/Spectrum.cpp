#include "base.h"
#include "Spectrum.h"
#include "WindowedFFT.h"

Spectrum::Spectrum(int const windowLength, double const sampleRate_) :
numBins(windowLength),
sampleRate(sampleRate_),
buffer(numBins)
{
}

Spectrum::~Spectrum()
{
    //DBG("Spectrum::~Spectrum()");
}

void Spectrum::clear()
{
    buffer.clear();
}

void Spectrum::calculate(FFT *fft, SpectrumWindow *window, float const *source)
{
    float const * const windowValues = window->getValues();
    int windowLength = window->getLength();
    
    jassert(windowLength <= numBins);
    
    clear();

    //
    // Apply FFT window and copy into fftBuffer
    //
    for (int i = 0; i < windowLength; ++i)
    {
        float binValue = source[i] * windowValues[i];
        
        setBin(i, binValue);
    }
    
    //
    // Transform time domain -> frequency domain
    //
    fft->performFrequencyOnlyForwardTransform(buffer.getFFTBuffer());
    
    //
    // Clear out mirrored results
    //
#if 0
    for (int i = EXTRA_BINS + numBins; i < fftBufferLength; ++i)
    {
        fftBuffer[i] = 0.0f;
    }
#endif
}

void Spectrum::calculate(FFT *fft, SpectrumWindow *window, AudioSampleBuffer &source, int const channel, int const startPosition)
{
    float const * const windowValues = window->getValues();
    int windowLength = window->getLength();
    
    jassert(windowLength <= numBins);
    jassert(windowLength <= source.getNumSamples());
    
    clear();
    
    //
    // Apply FFT window and copy into fftBuffer
    //
    float const *inputData = source.getReadPointer(channel, startPosition);
    for (int i = 0; i < windowLength; ++i)
    {
        setBin(i, inputData[i] * windowValues[i]);
    }
    
    //
    // Transform time doman -> frequency domain
    //
    fft->performFrequencyOnlyForwardTransform(buffer.getFFTBuffer());
    
    //
    // Clear out mirrored results
    //
#if 0
    for (int i = EXTRA_BINS + numBins; i < fftBufferLength; ++i)
    {
        fftBuffer[i] = 0.0f;
    }
#endif
}

void Spectrum::dump() const
{
    for (int i = 0; i < numBins; ++i)
    {
        double binFrequency = (double(i) / numBins) * sampleRate;
        
        if (binFrequency > 20000.0f)
            break;
        
        float const minusInfinity = -100.0f;
        float dB = Decibels::gainToDecibels(getBin(i), minusInfinity);
        if (dB > minusInfinity)
        {
            String dbString(Decibels::toString(dB, 1, minusInfinity));
            DBG("i:" << i << "   " <<  dbString << "   " << binFrequency << " Hz");
        }
    }
}

void Spectrum::binsToDecibels(Array<float> &decibelArray)
{
    float const minusInfinity = -300.0f;

    decibelArray.ensureStorageAllocated(numBins);
    
    for (int i = 0; i < numBins; ++i)
    {
        float dB = Decibels::gainToDecibels(buffer.getBin(i), minusInfinity);
        decibelArray.set(i, dB);
    }
}

double Spectrum::interpolatePeak(int const bin)
{
    float y1,y2,y3;
    double p;
    double ly1,ly2,ly3;
    double peak = 0.0f;
    
    // Do Quadratic peak interpolation ...
    y1 = getBin(bin - 1);
    y2 = getBin(bin);
    y3 = getBin(bin + 1);
    
    // Guard against illegal log ...
    if ((y1 == 0.0) || (y3 == 0.0))
    {
        // default to peak mag ...
        peak = y2;
    }
    else
    {
        // log of mag point values ...
        ly1 = 20.0 * log10(y1);
        ly2 = 20.0 * log10(y2);
        ly3 = 20.0 * log10(y3);
        
        // sub harmonic position of the peak ( -.5 to +.5 ) 
        p = (1.0 / 2.0) * (ly1 - ly3) / (ly1 - 2.0 * ly2 + ly3); 
        
        // interp mag peak value ...
        peak = y2 - (1.0 / 4.0) * (y1 - y3) * p;
    }

    return peak;
}

int Spectrum::getBinForFrequency(double const frequency) const
{
    double frequencyResolution = sampleRate / numBins;
    return roundDoubleToInt(frequency / frequencyResolution);
}

int Spectrum::getBinForHarmonic(double const fundamentalFrequency, int const harmonicOrder) const
{
    //DBG("Spectrum::getBinForHarmonic " << "frequencyResolution:" << frequencyResolution << " fundamentalFrequency:" << fundamentalFrequency << " harmonicOrder:" << harmonicOrder);
    
    double frequencyResolution = sampleRate / numBins;
    float harmonicFrequency;
    int k;
    //int fundamentalBin = roundDoubleToInt(fundamentalFrequency / frequencyResolution);

    // Calc target frequency ...
    harmonicFrequency = harmonicOrder * fundamentalFrequency;
    
    // Calc harmonic bin ...
    k = roundDoubleToInt(harmonicFrequency / frequencyResolution);
    
    // Break if past end of FFT array ...
    if (k >= numBins)
    {
        //DBG("Spectrum::getBinForHarmonic went past the end of the array, index:" << k);
        return -1;
    }
    
#if 0
    // Find peak at expected frequency location +/- 1 ..
    
    // Use "center" bin if it's equal to or greater to left and right ...
    float center = getBin(k);
    float left = getBin(k - 1);
    float right = getBin(k + 1);
    if (center >= left && center >= right)
    {
        //DBG("Spectrum::getBinForHarmonic center bin is equal to or greater than left/right bin, index:" << k);
        return k;
    }
    // left or right max?
    else if (left > right)
    {
        // check against -2
        if (left >= getBin(k - 2))
        {
            //DBG("Spectrum::getBinForHarmonic one bin to the left is the max, index:" << (k - 1));
            return k - 1;
        }
    }
    else
    {
        // right max
        
        // check againsts +2
        if (right >= getBin(k + 2))
        {
            //DBG("Spectrum::getBinForHarmonic one bin to the right is the max, index:" << (k + 1));
            return k + 1;
        }
    }
    
    //DBG("Spectrum::getBinForHarmonic giving up, couldn't find the correct bin");
    return -1;
#else
    return k;
#endif
}

void Spectrum::trimMirror()
{
    for (int i = numBins / 2; i < numBins; ++i)
    {
        setBin(i, 0.0f);
    }
}

double Spectrum::calculateTHD(double const fundamentalFrequency, int const firstHarmonic, int const lastHarmonic)
{
    Array<double> harmonicPeaks;
    int const maxBin = numBins / 2;
    
    //dump();
    
    //trimMirror();
    
    //
    // Clear the array
    //
    harmonicPeaks.ensureStorageAllocated(lastHarmonic + 1);
    for (int harmonicOrder = 0; harmonicOrder <= lastHarmonic; ++harmonicOrder)
    {
        harmonicPeaks.add(0.0);
    }
    
    {
        int harmonicOrder = 1; // fundamental frequency
        int bin = getBinForHarmonic( fundamentalFrequency, harmonicOrder);
        
        if (bin >= 0 && bin < maxBin)
        {
            double peak = interpolatePeak(bin);
            double peakDb = Decibels::gainToDecibels(peak, -100.0);
            DBG("harmonicOrder " << harmonicOrder << " bin:" << bin << "  peak:" << peak);
            DBG("    peak:" + Decibels::toString(peakDb));
            
            harmonicPeaks.set(harmonicOrder, peak);
        }
    }
    
    for (int harmonicOrder = firstHarmonic; harmonicOrder <= lastHarmonic; ++harmonicOrder)
    {
        int bin = getBinForHarmonic( fundamentalFrequency, harmonicOrder);
        
        if (bin >= 0 && bin < maxBin)
        {
            //double peak = interpolatePeak(bin);
            double peak = getBin(bin);
            double peakDb = Decibels::gainToDecibels(peak, -100.0);
            DBG("harmonicOrder " << harmonicOrder << " bin:" << bin << "  peak:" << peak);
            DBG("    peak:" + Decibels::toString(peakDb));
            
            harmonicPeaks.set(harmonicOrder, peak);
        }
    }
    
    //
    // Denominator is fundamental peak
    //
    double denominator = harmonicPeaks[1];
    if (0.0 == denominator)
    {
        return 1.0;
    }

    //
    // Numerator is RMS sum of upper harmonic peaks
    //
    double numerator = 0.0;
    for (int harmonicOrder = firstHarmonic; harmonicOrder <= lastHarmonic; ++harmonicOrder)
    {
        double peak = harmonicPeaks[harmonicOrder];
        numerator += peak * peak;
    }
    
    double thd = sqrt(numerator) / denominator;
    DBG("THD fundamental:" << fundamentalFrequency << "  THD(%):" << thd * 100.0 << "  THD(dB):" + Decibels::toString(Decibels::gainToDecibels(thd)));
    return thd;
}
