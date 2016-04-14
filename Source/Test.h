#pragma once

class ehw;
class ProductionUnit;

#include "ErrorCodes.h"
#include "FrequencySweepAudioSource.h"
#include "Upsampler.h"
#include "SpectrumAnalyzer.h"

class Test
{
protected:
	Test(XmlElement *xe, bool &ok, ProductionUnit *unit_);

public:
	static Test *Create(XmlElement *xe, int input, int output, bool &ok, ProductionUnit *unit_);
	virtual ~Test();
    
    virtual int getSamplesRequired();

	virtual void Setup( int samples_per_block,
				ToneGeneratorAudioSource &tone,
				uint32 &active_outputs);
    virtual void fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone);
	virtual bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes) = 0;
	
	String title;
	int input;
	int output;
	int num_channels;
	int sample_rate;
	float output_amplitude_db;	
	float output_frequency;
	float pass_threshold_db;
	float min_level_db;
	float max_level_db;
	float minSampleRate;
	float maxSampleRate;
    float glitchThreshold;
	ProductionUnit* unit;
    uint32 requiredTestAdapterProductId;

protected:
	String MsgSampleRate();
};



class ThdnTest : public Test
{
public:
	ThdnTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~ThdnTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
};


class DiffThdnTest : public Test
{
public:
	DiffThdnTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~DiffThdnTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
};


class DynRangeTest : public Test
{
public:
	DynRangeTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~DynRangeTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
};


class DiffDynRangeTest : public Test
{
public:
	DiffDynRangeTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~DiffDynRangeTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
};


class FrequencyResponseTest : public Test
{
public:
	FrequencyResponseTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~FrequencyResponseTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);

};

class FrequencySweepResponseTest : public Test
{
public:
    FrequencySweepResponseTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
    ~FrequencySweepResponseTest();
    
    virtual int getSamplesRequired();
    virtual void Setup( int samples_per_block,
                       ToneGeneratorAudioSource &tone,
                       uint32 &active_outputs);
    virtual void fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone);
    bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
    
protected:
	bool getFreq(int &period_start,
		int &period,
		double &amplitude);

    FrequencySweepAudioSource sweepGenerator;
    float sweep_time_seconds;
    float sweep_delay_seconds;
	float sweep_fadein_seconds;
	float sweep_fadeout_seconds;
    float sweep_record_seconds;
	Upsampler upsampler;
};


class LevelCheckTest : public Test
{
public:
	LevelCheckTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~LevelCheckTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);

    float peakRejectThresholdDb;
};

class RelativeLevelTest : public Test
{
public:
	RelativeLevelTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~RelativeLevelTest();
    
    bool calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes);
    
    bool autoPass;
    static const Identifier relativeLevelResults;
};


class HexInputCrosstalkTest : public Test
{
public:
	HexInputCrosstalkTest(XmlElement *xe,bool &ok, ProductionUnit *unit_);
	~HexInputCrosstalkTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes);
};

class SaturationTest : public Test
{
public:
	SaturationTest(XmlElement *xe,bool &ok, ProductionUnit *unit_);
	~SaturationTest();

	bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes);

};

class PhaseTest : public Test
{
public:
	PhaseTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
	~PhaseTest();

    bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes);
};

class SilenceTest : public Test
{
public:
    SilenceTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
    ~SilenceTest();
    
    bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes);
};

class FrequencyIsolationTest : public Test
{
public:
    FrequencyIsolationTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
    ~FrequencyIsolationTest();
    
    bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes);
    
    virtual int getSamplesRequired()
    {
        return sample_rate;
    }
    
protected:
    SpectrumAnalyzer analyzer;
};

class ReferenceVoltageTest : public Test
{
public:
    ReferenceVoltageTest(XmlElement *xe, bool &ok, ProductionUnit *unit_);
    
    virtual void fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone) override;
    virtual int getSamplesRequired() override;
    virtual bool calc(OwnedArray<AudioSampleBuffer> &buffs,String &msg, ErrorCodes &errorCodes) override;
    
protected:
    float squareWaveMinAmplitude;
    float squareWaveMaxAmplitude;
    float squareWaveFrequency;
    int squareWavePeriodSamples;
    int squareWavePosition;
    
    struct SquareWaveAnalysisResult
    {
        SquareWaveAnalysisResult()
        {
            clear(0.0f);
        }
        
        void add(float sample)
        {
            average += sample;
            min = jmin(min, sample);
            max = jmax(max, sample);
        }
        
        void clear(float const sign)
        {
            if (sign < 0.0f)
            {
                min = 0.0f;
                max = -FLT_MAX;
                average = 0.0f;
            }
            else
            {
                min = FLT_MAX;
                max = 0.0f;
                average = 0.0f;
            }
        }
        
        
        float min;
        float max;
        float average;
    };
    
    
    void findZeroCrossing(const float * data, int numSamples, int startIndex, int &zeroCrossingIndex);
    Result analyze(
                   String const name,
                   const float *data,
                   int numSamples,
                   float &totalResult,
                   Range<float> const range
                   );
};
