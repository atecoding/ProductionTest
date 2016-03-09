#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

FrequencyIsolationTest::FrequencyIsolationTest(XmlElement *xe, bool &ok, ProductionUnit *unit_) :
    Test(xe,ok,unit_)
{
    ok &= getFloatValue(xe, "output_frequency", output_frequency);
    ok &= getFloatValue(xe, T("min_level_db"), min_level_db);
    ok &= getFloatValue(xe, T("max_level_db"), max_level_db);
}

FrequencyIsolationTest::~FrequencyIsolationTest()
{
    
}

bool FrequencyIsolationTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
    int channel;
    bool pass = true;
    float const minusInfinityDB = -144.0f;

    msg = String::formatted("Frequency isolation %.1f Hz at ", output_frequency);
    msg += MsgSampleRate();
    msg += ": ";
    msg += newLine;
    
    int const numSpectrumSamples = sample_rate / 2;
    for (channel = 0; channel < num_channels; channel++)
    {
        AudioSampleBuffer* channelBuffer = buffs[input + channel];
        AudioBuffer<float> tempBuffer(channelBuffer->getArrayOfWritePointers(),
                                         1, // 1 channel
                                      0, numSpectrumSamples);
        
        analyzer.calculate(tempBuffer, 0, sample_rate);
        Spectrum const *spectrum( analyzer.getAverageSpectrum());
        //analyzer.dump();
        int bin = spectrum->getBinForFrequency(output_frequency);
        float amplitude = spectrum->getBin(bin);
        float dB = Decibels::gainToDecibels(amplitude, minusInfinityDB);
        msg += String::formatted("   Channel %d: ", channel + input + 1);
        msg += Decibels::toString( dB, 1, minusInfinityDB);
        if (dB < min_level_db || dB > max_level_db)
        {
            msg += " - FAIL";
            pass = false;
            errorCodes.add(ErrorCodes::LEVEL, channel);
        }
        msg += newLine;
        //DBG(String::formatted("channel:%d  amp:%f", channel, amplitude));
    }
    
    return pass;
}
