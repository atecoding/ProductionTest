#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

SilenceTest::SilenceTest(XmlElement *xe, bool &ok, ProductionUnit *unit_) :
Test(xe,ok,unit_)
{
    getFloatValue(xe, "output_frequency", output_frequency);
}

SilenceTest::~SilenceTest()
{
    
}

bool SilenceTest::calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes)
{
    int channel;
    bool pass = true;

    msg = String::formatted("Silence check at ");
    msg += MsgSampleRate();
    msg += ": ";
    
    for (channel = 0; channel < num_channels; channel++)
    {
        AudioSampleBuffer* buffer = buffs[input + channel];
        float peak = buffer->getMagnitude(0, 0, buffer->getNumSamples());
        DBG(channel << " " << peak);
        if (peak > 0.0f)
        {
            msg += "Signal detected on input " + String(input + channel + 1);
            pass = false;
        }
    }
    
    return pass;
}
