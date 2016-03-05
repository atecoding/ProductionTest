#include "base.h"
#include "SpectrumAnalyzer.h"
//#include "FrequencySteppedSweepAudioSource.h"
#include "WaveFile.h"

#define SPECTRUM_ANALYZER_IDENTIFIER(name) const Identifier SpectrumAnalyzer::Identifiers:: name(#name);
SPECTRUM_ANALYZER_IDENTIFIER_LIST

static String const blackmanHarris4TermWindowType("Blackman-Harris 4-term");
static String const blackmanHarris7TermWindowType("Blackman-Harris 7-term");

SpectrumAnalyzer::SpectrumAnalyzer() :
windowType(blackmanHarris4TermWindowType),
windowLengthSamples(2048),
sampleRate(48000.0)
/*,upsampledSampleRate(48000.0)*/
{
    windowOverlapSamples = roundDoubleToInt(windowLengthSamples * 0.661);
}

static Result missingProperty(Identifier identifier)
{
    return Result::fail(identifier.toString() + " property not found");
}

static Result invalidPropertyResult(Identifier identifier, var value)
{
    return Result::fail("Invalid value " + value.toString() + " for property " + identifier.toString());
}

Result SpectrumAnalyzer::validate(NamedValueSet &values)
{
    if (false == values.contains(Identifiers::WindowType))
    {
        return missingProperty(Identifiers::WindowType);
    }
    
    if (false == values.contains(Identifiers::WindowLengthSamples))
    {
        return missingProperty(Identifiers::WindowType);
    }
    
    if (false == values.contains(Identifiers::WindowOverlapPercent))
    {
        return missingProperty(Identifiers::WindowType);
    }
    
    String windowType_(values[Identifiers::WindowType].toString());
    if (windowType_ != blackmanHarris4TermWindowType &&
        windowType_ != blackmanHarris7TermWindowType)
    {
        return invalidPropertyResult(Identifiers::WindowType, windowType_);
    }
    
    int windowLengthSamples_ = values[Identifiers::WindowLengthSamples];
    if (windowLengthSamples_ < 2 || false == isPowerOfTwo(windowLengthSamples_))
    {
        return invalidPropertyResult(Identifiers::WindowLengthSamples, windowLengthSamples_);
    }
    
    double windowOverlapPercent_ = values[Identifiers::WindowOverlapPercent];
    if (windowOverlapPercent_ <= 0.0 || windowOverlapPercent_ > 100.0)
    {
        return invalidPropertyResult(Identifiers::WindowOverlapPercent, windowOverlapPercent_);
    }
    
    return Result::ok();
}

Result SpectrumAnalyzer::configure(var const configuration_)
{
    DynamicObject * const configurationObject = configuration_.getDynamicObject();

    if (nullptr == configurationObject)
    {
        return Result::fail("Configuration must be an object");
    }
    
    NamedValueSet &configurationValues(configurationObject->getProperties());
    Result validationResult(validate(configurationValues));
    if (validationResult.failed())
        return validationResult;
    
    windowType = configurationValues[Identifiers::WindowType];
    windowLengthSamples = configurationValues[Identifiers::WindowLengthSamples];
    windowOverlapSamples = roundDoubleToInt(windowLengthSamples * 0.01 * (double)configurationValues[Identifiers::WindowOverlapPercent]);
    
    //upsampledSampleRate = configurationValues[Identifiers::SampleRate];
    
    return Result::ok();
}

void SpectrumAnalyzer::calculate(AudioBuffer<float> const &inputData, int const channel, double const inputSampleRate_)
{
    jassert(inputData.getNumSamples() >= windowLengthSamples);
    
#if 0
    AudioBuffer<float> audioBufferFloat(2, inputData.getNumSamples() * 2);
    audioBufferFloat.clear();
    audioBufferFloat.copyFrom(0, // destChannel
                              0, // destStartSample
                              inputData, // source
                              channel,  // sourceChannel
                              0, // sourceStartSample
                              inputData.getNumSamples());
#endif
    
    //
    // Make the FFT & window objects
    //
    if (nullptr == fft || fft->getSize() != windowLengthSamples)
    {
        int order = roundDoubleToInt( log10(windowLengthSamples) / log10(2.0) );
        fft = new FFT(order, false);
        
        window = createWindow(windowLengthSamples);
        if (nullptr == window)
            return;
        
        //window->dump();
    }
    
    //
    // Make the upsampler object
    //
#if 0
    if (nullptr == upsampler || upsampler->getInputSampleRate() != inputSampleRate || upsampler->getOutputSampleRate() != upsampledSampleRate)
    {
        upsampler = new Upsampler(inputSampleRate, upsampledSampleRate);
    }

    int outputSampleCount = roundDoubleToInt( inputData.getNumSamples() * upsampledSampleRate / inputSampleRate ) + 1;
    AudioBuffer<float> upsampledData(1, outputSampleCount);
    upsampler->upsample(inputData, channel, upsampledData, 0, outputSampleCount);
#else
    sampleRate = inputSampleRate_;
#endif
    
#if 0
    audioBufferFloat.copyFrom(1, // destChannel
                              0, // destStartSample
                              upsampledData, // source
                              0,  // sourceChannel
                              0, // sourceStartSample
                              outputSampleCount);
    File wavefile(File::getCurrentWorkingDirectory().getChildFile("spectrum.wav"));
    WriteWaveFile(wavefile, 48000, &audioBufferFloat);
#endif

    //
    // Reset any previously created spectrum containers
    //
    int spectrumIndex;
    for (spectrumIndex = 0; spectrumIndex < spectrums.size(); ++spectrumIndex)
    {
        spectrums[spectrumIndex]->clear();
    }
    
    //
    // Calculate the overlapping FFTs and store them
    //
    int position = 0;
    spectrumIndex = 0;
    while (position < (inputData.getNumSamples() - windowLengthSamples))
    {
        Spectrum* spectrum = spectrums[spectrumIndex];
        
        if (nullptr == spectrum)
        {
            spectrum = new Spectrum(windowLengthSamples, sampleRate);
        }
        
        spectrum->calculate(fft, window, inputData.getReadPointer(channel, position));
        spectrums.set(spectrumIndex, spectrum);
        
        position += windowOverlapSamples;
        spectrumIndex++;
    }
    
    //
    // Create the average spectrum container if necessary
    //
    if (nullptr == averageSpectrum || averageSpectrum->getNumBins() != windowLengthSamples)
    {
        averageSpectrum = new Spectrum(windowLengthSamples, sampleRate);
    }
    
    //
    // Average the spectrums
    //
    averageSpectrum->clear();
    for (int spectrumIndex = 0; spectrumIndex < spectrums.size(); ++spectrumIndex)
    {
        Spectrum* spectrum = spectrums[spectrumIndex];
        for (int bin = 0; bin < windowLengthSamples * 2; ++bin)
        {
            float binValue = averageSpectrum->getBin(bin);
            averageSpectrum->setBin(bin, binValue + spectrum->getBin(bin));
        }
    }
    
    float const reciprocal = 1.0f / spectrums.size();
    for (int bin = 0; bin < windowLengthSamples * 2; ++bin)
    {
        float binValue = averageSpectrum->getBin(bin);
        averageSpectrum->setBin(bin, binValue * reciprocal);
    }
}

SpectrumWindow* SpectrumAnalyzer::createWindow(int const order)
{
    if (blackmanHarris4TermWindowType == windowType)
    {
        return new BlackmanHarrisWindow(order, 4);
    }
    
    if (blackmanHarris7TermWindowType == windowType)
    {
        return new BlackmanHarrisWindow(order, 7);
    }
    
    jassertfalse;
    return nullptr;
}


void SpectrumAnalyzer::dump()
{
    DBG("Spectrum count:" << spectrums.size());
    DBG("Sample rate:" << sampleRate);
    
    /*
    if (window)
    {
        window->dump();
        DBG("\n\n");
    }
    */
    
    for (int i = 0; i < spectrums.size(); ++i)
    {
        DBG("\nSpectrum " << i);
        Spectrum* spectrum = spectrums[i];
        if (spectrum)
            spectrum->dump();
    }
    
    DBG("SpectrumAnalyzer::dump done");
}

void SpectrumAnalyzer::testPureTone(var const configuration_, double sampleRate, double toneFrequency)
{
    SpectrumAnalyzer analyzer;
    
    Result result(analyzer.configure(configuration_));
    if (result.failed())
    {
        DBG("SpectrumAnalyzer::testPureTone error " + result.getErrorMessage());
        return;
    }
    
    int blockSamples = 1024;
    ToneGeneratorAudioSource tone1, tone2;
    tone1.setAmplitude(1.0f);
    tone1.setFrequency(toneFrequency);
    tone1.prepareToPlay(blockSamples, sampleRate);
    tone2.setAmplitude(1.0f);
    tone2.setFrequency(toneFrequency * 1.5);
    tone2.prepareToPlay(blockSamples, sampleRate);
    
    int numSamples = roundDoubleToInt(sampleRate * 4.0);
    AudioBuffer<float> buffer(1, numSamples);
    AudioBuffer<float> buffer1(1, numSamples);
    buffer.clear();
    buffer1.clear();
    
    int samplesRemaining = buffer.getNumSamples();
    AudioSourceChannelInfo channelInfo(&buffer, 0, blockSamples);
    AudioSourceChannelInfo channelInfo1(&buffer1, 0, blockSamples);
    while (samplesRemaining > 0)
    {
        blockSamples = jmin( blockSamples, samplesRemaining);
        
        channelInfo.numSamples = blockSamples;
        tone1.getNextAudioBlock(channelInfo);

        channelInfo1.numSamples = blockSamples;
        tone2.getNextAudioBlock(channelInfo1);
        
        channelInfo.startSample += blockSamples;
        channelInfo1.startSample += blockSamples;
        samplesRemaining -= blockSamples;
    }
    
    buffer.addFrom(0, 0, buffer1, 0, 0, buffer1.getNumSamples(), 1.0f);
    
    int const channel = 0;
    analyzer.calculate(buffer, channel, sampleRate);
    analyzer.dump();
}

void SpectrumAnalyzer::testSteppedSweep(var const analyzerConfiguration_, var const sweepConfiguration_, double sampleRate, double toneFrequency)
{
#if 0
    SpectrumAnalyzer analyzer;
    
    Result result(analyzer.configure(analyzerConfiguration_));
    if (result.failed())
    {
        DBG("SpectrumAnalyzer::testSteppedSweep error " + result.getErrorMessage());
        return;
    }
    
    int blockSamples = 1024;
    FrequencySteppedSweepAudioSource sweep;
    sweep.configure(sweepConfiguration_);
    sweep.prepareToPlay(blockSamples, sampleRate);
    
    int numSamples = sweep.getTotalSamples() + roundDoubleToInt(sampleRate * 0.25);
    AudioBuffer<float> buffer(1, numSamples);
    buffer.clear();
    
    int samplesRemaining = buffer.getNumSamples();
    AudioSourceChannelInfo channelInfo(&buffer, 0, blockSamples);
    while (samplesRemaining > 0)
    {
        blockSamples = jmin( blockSamples, samplesRemaining);
        
        channelInfo.numSamples = blockSamples;
        sweep.getNextAudioBlock(channelInfo);
        
        channelInfo.startSample += blockSamples;
        samplesRemaining -= blockSamples;
    }
    
    int const channel = 0;
    analyzer.calculate(buffer, channel, sampleRate);
    analyzer.dump();
#endif
}

void SpectrumAnalyzer::exportCSV(File csvFile)
{
    Spectrum* spectrum = spectrums[0];
    if (nullptr == spectrum)
        return;

    int const numBins = spectrum->getNumBins();
    String const comma(",");
    String text;
    
    {
        String line(comma + comma);
        for (int spectrumIndex = 0; spectrumIndex < spectrums.size(); ++spectrumIndex)
        {
            line += String(spectrumIndex);
            if (spectrumIndex < (spectrums.size() - 1))
                line += comma + comma;
        }
        
        text += line + newLine;
    }
    
    for (int bin = 0; bin < numBins / 2; ++bin) // Only print out below Nyquist
    {
        String line(bin);
        
        line += comma;
        double frequency = (bin * sampleRate) / numBins;
        line += String(frequency,1);
        line += comma;
        
        float const minusInfinityDb = -100.0f;
        for (int spectrumIndex = 0; spectrumIndex < spectrums.size(); ++spectrumIndex)
        {
            spectrum = spectrums[spectrumIndex];
            float binValue = spectrum->getBin(bin);
            float binDb = Decibels::gainToDecibels(binValue, minusInfinityDb);
            line += String(binValue, 8) + comma;
            if (binDb > minusInfinityDb)
            {
                line += Decibels::toString(binDb, 1, minusInfinityDb);
            }

            line += comma;
        }
        
        {
            spectrum = averageSpectrum;
            float binValue = spectrum->getBin(bin);
            float binDb = Decibels::gainToDecibels(binValue, minusInfinityDb);
            line += String(binValue, 8) + comma;
            if (binDb > minusInfinityDb)
            {
                line += Decibels::toString(binDb, 1, minusInfinityDb);
            }
        }
        
        text += line + newLine;
    }
    
    csvFile.appendText(text);
}

