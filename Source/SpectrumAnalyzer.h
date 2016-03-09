#pragma once

#include "Spectrum.h"
#include "WindowedFFT.h"
#include "Upsampler.h"

#define USE_FFTW 0

#if USE_FFTW
#include "fftw3.h"
#endif

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    
    Result configure(var const configuration_);
    void calculate(AudioBuffer<float> const &inputData, int const channel, double const inputSampleRate);
    void exportCSV(File csvFile);
    
    #define SPECTRUM_ANALYZER_IDENTIFIER_LIST \
    SPECTRUM_ANALYZER_IDENTIFIER( WindowType       ) \
    SPECTRUM_ANALYZER_IDENTIFIER( WindowLengthSamples        ) \
    SPECTRUM_ANALYZER_IDENTIFIER( WindowOverlapPercent       ) \
    SPECTRUM_ANALYZER_IDENTIFIER( SampleRate       )
    
    struct Identifiers
    {
#define SPECTRUM_ANALYZER_IDENTIFIER(name) static const Identifier name;
        SPECTRUM_ANALYZER_IDENTIFIER_LIST
#undef SPECTRUM_ANALYZER_IDENTIFIER
    };
    
    static void testPureTone(var const configuration_, double sampleRate, double toneFrequency);
    static void testSteppedSweep(var const analyzerConfiguration_, var const sweepConfiguration_, double sampleRate, double toneFrequency);
    void dump();
    
    OwnedArray<Spectrum> const &getSpectra() const
    {
        return spectra;
    }
    
    Spectrum * const getAverageSpectrum() const
    {
        return averageSpectrum;
    }
    
private:
    String windowType;
    int windowLengthSamples;
    int windowOverlapSamples;
    double sampleRate;
    //double upsampledSampleRate;
    
    //ScopedPointer<Upsampler> upsampler;
#if USE_FFTW
#else
    ScopedPointer<FFT> fft;
#endif
    ScopedPointer<SpectrumWindow> window;
    OwnedArray<Spectrum> spectra;
    ScopedPointer<Spectrum> averageSpectrum;
    
    Result validate(NamedValueSet &values);
    SpectrumWindow* createWindow(int const order);
};
