#include "base.h"
#include "Test.h"
#include "Analysis.h"
#include "wavefile.h"
#include "xml.h"
#include "ProductionUnit.h"

static const double upsamplerOutputRate = 384000.0;
static const double minusInfinity = -144.0f;

FrequencySweepResponseTest::FrequencySweepResponseTest(XmlElement *xe, bool &ok, ProductionUnit *unit_) :
Test(xe, ok, unit_),
sweep_time_seconds(2.0f),
sweep_delay_seconds(0.5f),
sweep_fadein_seconds(0.3f),
sweep_fadeout_seconds(0.1f),
sweep_record_seconds(2.9f),
upsampler(sample_rate, upsamplerOutputRate)
{
	ok &= getFloatValue(xe, "pass_threshold_db", pass_threshold_db);
	ok &= getFloatValue(xe, "output_frequency"
		, output_frequency);
	ok &= getFloatValue(xe, "sweep_time_seconds", sweep_time_seconds);
	ok &= getFloatValue(xe, "sweep_delay_seconds", sweep_delay_seconds);
	ok &= getFloatValue(xe, "sweep_fadein_seconds", sweep_fadein_seconds);
	ok &= getFloatValue(xe, "sweep_record_seconds", sweep_record_seconds);
	ok &= getFloatValue(xe, "sweep_fadeout_seconds", sweep_fadeout_seconds);

	upsampler.setOutputBufferSize(sweep_record_seconds);
}


FrequencySweepResponseTest::~FrequencySweepResponseTest()
{
	sweepGenerator.releaseResources();
}

int FrequencySweepResponseTest::getSamplesRequired()
{
	return roundFloatToInt(sample_rate * (sweep_delay_seconds + sweep_fadein_seconds + sweep_time_seconds + sweep_fadeout_seconds));
}

void FrequencySweepResponseTest::Setup(int samples_per_block, ToneGeneratorAudioSource &tone,
	uint32 &active_outputs)
{
	Test::Setup(samples_per_block, tone, active_outputs);

	sweepGenerator.setSweepTime(sweep_delay_seconds, sweep_fadein_seconds, sweep_fadeout_seconds, sweep_time_seconds);
	sweepGenerator.prepareToPlay(samples_per_block, sample_rate);
}

void FrequencySweepResponseTest::fillAudioOutputs(AudioSampleBuffer &buffer, ToneGeneratorAudioSource &tone)
{
	AudioSourceChannelInfo asci;
	asci.buffer = &buffer;
	asci.numSamples = buffer.getNumSamples();
	asci.startSample = 0;
	sweepGenerator.getNextAudioBlock(asci);
}

bool FrequencySweepResponseTest::calc(OwnedArray<AudioSampleBuffer> &buffs, String &msg, ErrorCodes &errorCodes)
{
	bool pass[4] = { TRUE, TRUE, TRUE, TRUE };

	msg = String("Frequency Response (20Hz-20kHz):") + newLine;

	for (int channel = 0; channel < num_channels; channel++)
	{
		int physicalInput = input + channel;

		upsampler.upsample(buffs[physicalInput]);

		bool result;
		int period = roundDoubleToInt( upsamplerOutputRate / 18 );
		double amplitude = 0.0;
		double max_db, min_db;
		int period_start = 0;
		int periods = 0;

		struct frequency
		{
			int		location;
			int		period;
			double	amplitude;
		};

		frequency reference, max, min;

		// Scan output buffer to find start of waveform

		result = getFreq(period_start, period, amplitude);
		if (FALSE == result)
		{
			pass[channel] = FALSE;
			msg += "    *** Input " + String(physicalInput + 1) + ":";
			msg += String("Invalid Waveform!") + newLine;

			errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
		}
		period_start += period;

		period = 20000;

		// Scan output buffer to find 20Hz frequency

		while (period > upsamplerOutputRate / 20)
		{
			result = getFreq(period_start, period, amplitude);
			if (FALSE == result)
			{
				pass[channel] = FALSE;
				msg += "    *** Input " + String(physicalInput + 1) + ":";
				msg += String("Invalid Waveform!") + newLine;
				errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
				break;
			}
			period_start += period;
		}

		max.period = period;
		max.amplitude = amplitude;
		max.location = period_start - period;
		min.period = period;
		min.amplitude = amplitude;
		min.location = period_start - period;

		// Scan output buffer up to 1kHz

		while (pass[channel] == TRUE && period > upsamplerOutputRate / 1000)
		{
			result = getFreq(period_start, period, amplitude);
			if (FALSE == result)
			{
				pass[channel] = FALSE;
				msg += "    *** Input " + String(physicalInput + 1) + ":";
				msg += String("Invalid Waveform!") + newLine;
				errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
				break;
			}
			if (amplitude > max.amplitude)
			{
				max.amplitude = amplitude;
				max.period = period;
				max.location = period_start;
			}
			if (amplitude < min.amplitude)
			{
				min.amplitude = amplitude;
				min.period = period;
				min.location = period_start;
			}
			period_start += period;
			periods++;
		}
		reference.amplitude = amplitude;
		reference.period = period;
		reference.location = period_start - period;

		// Scan output buffer up to 20kHz

		while (pass[channel] == TRUE && period > upsamplerOutputRate / 20000)
		{
			result = getFreq(period_start, period, amplitude);
			if (FALSE == result)
			{
				pass[channel] = FALSE;
				msg += "    *** Input " + String(physicalInput + 1) + ":";
				msg += String("Invalid Waveform!") + newLine;
				errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
				break;
			}
			if (amplitude > max.amplitude)
			{
				max.amplitude = amplitude;
				max.period = period;
				max.location = period_start;
			}
			if (amplitude < min.amplitude)
			{
				min.amplitude = amplitude;
				min.period = period;
				min.location = period_start;
			}
			period_start += period;
			periods++;
		}

		if (pass[channel] == TRUE)
		{
			if (reference.amplitude > 0.02)
			{
				max_db = Decibels::gainToDecibels(max.amplitude / reference.amplitude, minusInfinity);
				min_db = Decibels::gainToDecibels(min.amplitude / reference.amplitude, minusInfinity);
				if (max_db > pass_threshold_db || -1.0 * min_db > pass_threshold_db)
				{
					msg += "    *** Input " + String(physicalInput + 1) + ":";
					pass[channel] = FALSE;
					errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
				}
				else
					msg += "    Input " + String(physicalInput + 1) + ":";
				msg += String::formatted(" max: ", output, physicalInput);
				msg += Decibels::toString(max_db, 2, minusInfinity);
				msg += " min: ";
				msg += Decibels::toString(min_db, 2, minusInfinity) + newLine;
				//				msg += String::formatted("    1kHz: %d, %d, Min: %d, %d, Max: %d, %d", 
				//					reference.location, 384000 / reference.period, min.location, 384000 / min.period, max.location, 384000 / max.period) + newLine;
			}
			else
			{
				pass[channel] = FALSE;
				msg += "    *** Input " + String(physicalInput + 1) + ":";
				msg += String(" Invalid Waveform!") + newLine;
				errorCodes.add(ErrorCodes::LEVEL, physicalInput + 1);
			}
		}


#if WRITE_WAVE_FILES
		{
			if (FALSE == pass[channel])
			{
				String name;

				name = String::formatted("Frequency sweep out%02d-in%02d at %d Hz.wav", output, physicalInput, sample_rate);
				WriteWaveFile(unit, name, sample_rate, buffs[physicalInput], getSamplesRequired());

				name = String::formatted("Upsampled out%02d-in%02d at %d Hz.wav", output, physicalInput, upsamplerOutputRate);
				int upsampleWaveFileCount = jmin(upsampler.outputSampleCount, getSamplesRequired() * roundDoubleToInt(upsamplerOutputRate));
				WriteWaveFile(unit, name, sample_rate, upsampler.outputBuffer, upsampleWaveFileCount);
			}
		}
#endif
	}

	return pass[0] && pass[1] && pass[2] && pass[3];
}


bool FrequencySweepResponseTest::getFreq(int &period_start, int &period, double &amplitude)
{
	int i;
	double max = 0.0;
	double min = 0.0;

	if (period_start == 0)  // find start of waveform
	{
		for (i = 0; i < upsampler.outputSampleCount / 2; i++)
		{
			if (upsampler.outputBuffer[i] > max)
				max = upsampler.outputBuffer[i];
		}
		while (fabs(upsampler.outputBuffer[period_start]) < max * 0.5)
		{
			period_start++;
			if (period_start > upsampler.outputSampleCount / 4)
				return FALSE;
		}
		max = 0.0;
	}
	for (i = period_start + period / 6; i < upsampler.outputSampleCount - 2; i++)
	{
		if (upsampler.outputBuffer[i] > max)
			max = upsampler.outputBuffer[i];
		if (upsampler.outputBuffer[i] < min)
			min = upsampler.outputBuffer[i];
		if ((upsampler.outputBuffer[i] < 0.0) && (upsampler.outputBuffer[i + 1] > 0.0) && (upsampler.outputBuffer[i - period / 8] < 0.0) && (upsampler.outputBuffer[i + period / 8] > 0.0))
			break;
	}
	period = i - period_start;
	amplitude = max - min;
	if (amplitude == 0.0)
		return FALSE;

	return TRUE;
}
