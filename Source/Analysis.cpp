#include "base.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include "FilterCoeff.h"
#include "Analysis.h"
#include "wavefile.h"

// global:  aweightedData
// usage:   Used to store A-weighted input data
double aweightedData[4096-512];

// global:  signalBuffer
// usage:   Used to store signal data
double signalBuffer[1024];

// global:  noise
// usage:   Used to store noise data
double noise[1024];

double bandRejectResults[2048];

void RunFIRFilter(float const *audio,int num_samples,double const *taps,int num_taps,double *dest)
{
	int i,j;
	double mac;
	double const *tap;
    
	for (i = 0; i < num_samples; i++)
	{
		mac = 0;
        float const *src = audio + i;
		tap = taps;
		for (j = 0; j < num_taps; j++)
		{
			mac += *src * *tap;
			src++;
			tap++;
		}

		*dest = mac;
		dest++;
	}
}

void RunFIRFilter(double const *audio,int num_samples,double const *taps,int num_taps,double *dest)
{
	int i,j;
	double mac;
	double const *tap;

	for (i = 0; i < num_samples; i++)
	{
		mac = 0;
		double const *src = audio + i;
		tap = taps;
		for (j = 0; j < num_taps; j++)
		{
			mac += *src * *tap;
			src++;
			tap++;
		}

		*dest = mac;
		dest++;
	}
}

// computeTHDN
// Compute the THD+N of the given audio data
// The filters require that at least 2,560 samples are provided.
double computeTHDN(float const *input,int rate)
{
    int filterIndex,i;
    double rmsSignal,rmsNoise,thdn;
    double const *aWeight,*bandPass,*bandReject;

    // determine the filter index
    switch (rate)
    {
    	case 44100 :
      	filterIndex = FILTER_INDEX_44100;
         break;

    	case 48000 :
      	filterIndex = FILTER_INDEX_48000;
         break;

    	case 96000 :
      	filterIndex = FILTER_INDEX_96000;
         break;

      default :
      	assert(false);
		return 0.0;
	}

    // save the A-weighting, bandpass, and bandreject filters
    aWeight = A_Weight[filterIndex];
    bandPass = Bandpass_1kHz[filterIndex];
	bandReject = BandRjct_1kHz[filterIndex];

    // filter the data to create A-weighted signal
	RunFIRFilter(input,2048,aWeight,512,aweightedData);
	//WriteWaveFile("aweight.wav",rate,aweightedData,2048);

    // filter the data to create signal
	RunFIRFilter(aweightedData,1024,bandPass,512,signalBuffer);
	//WriteWaveFile("bandpass.wav",rate,signal,1024);

    // filter the data to create noise, first pass
	RunFIRFilter(aweightedData,2048-512,bandReject,512,bandRejectResults);
	//WriteWaveFile("reject1.wav",rate,bandRejectResults,2048-512);

    // filter the data to create noise, second pass
	RunFIRFilter(bandRejectResults,1024,bandReject,512,noise);
	//WriteWaveFile("noise.wav",rate,noise,1024);

    // sum up signal and noise
    rmsSignal = 0;
	rmsNoise = 0;
    for(i=0;i < 1024;i++) {
        rmsSignal += signalBuffer[i] * signalBuffer[i];
        rmsNoise += noise[i] * noise[i];
        }

    // finish computing rms values
    rmsSignal = sqrt((double)rmsSignal / 1024);
    rmsNoise = sqrt((double)rmsNoise / 1024);

   // check if the signal is zero
    if (rmsSignal == 0) 
		return 0.0;

    // calculate THD+N
    thdn = 20.0 * log10(rmsNoise / rmsSignal);
	return thdn;

#if 0
    // calculate amplitude (magic # = (2^31)(sqrt(2)/2)(10^(-2/20)))
    result->ampL = 8.0 + 20.0 * log10(rmsSignalL / 1206187622L);
    result->ampR = 8.0 + 20.0 * log10(rmsSignalR / 1206187622L);

    // fudge the amplitude
    // fixme ... result->ampL += device->getDeviceInfo(INFO_FUDGE_AMP);
    //result->ampR += device->getDeviceInfo(INFO_FUDGE_AMP);

    // return if we don't need to compute max amplitude
    if (saveMax == false)
        return;

    // save the maximum amplitude for the left and right channels
	 for (i=0,result->maxAmpL=0,result->maxAmpR=0;i < 2560;i++) {
			int l,r;

			l = abs( inputDataL[i] );
			r = abs( inputDataR[i] );

		  if (l > result->maxAmpL)
				result->maxAmpL = l;
		  if (r > result->maxAmpR)
				result->maxAmpR = r;
        }
#endif
}

// computeTHDN of Differential Input to two channels
// Compute the THD+N of the given audio data
// The filters require that at least 2,560 samples are provided.
double computeDiffTHDN(float const *input1, float const *input2, int rate, float const toneFrequency)
{
	int filterIndex,i;
	double rmsSignal,rmsNoise,thdn;
	double const *aWeight,*bandPass,*bandReject;

	// determine the filter index
	switch (rate)
	{
	case 44100 :
		filterIndex = FILTER_INDEX_44100;
		break;

	case 48000 :
		filterIndex = FILTER_INDEX_48000;
		break;

	case 96000:
		filterIndex = FILTER_INDEX_96000;
		break;

	default:
		assert(false);
		return 0.0;
	}

	// save the A-weighting, bandpass, and bandreject filters
	aWeight = A_Weight[filterIndex];

	if (1000.0 == toneFrequency)
	{
		bandPass = Bandpass_1kHz[filterIndex];
		bandReject = BandRjct_1kHz[filterIndex];
	}
	else if (7200.0 == toneFrequency)
	{
		bandPass = Bandpass_7200Hz;
		bandReject = BandRjct_7200Hz;
	}
	else
	{
		DBG("Really?");
		jassertfalse;
		return 0.0;
	}

	// Create a single differential signal from both inputs
    float differentialSignal[THDN_SAMPLES_REQUIRED];
	for (i = 0; i < THDN_SAMPLES_REQUIRED; i++)
	{
		differentialSignal[i] = (input1[i] - input2[i])/2;
	}

	// filter the data to create A-weighted signal
	RunFIRFilter(differentialSignal, 2048, aWeight, 512, aweightedData);
	//WriteWaveFile("aweight.wav",rate,aweightedData,2048);

	// filter the data to create signal
	RunFIRFilter(aweightedData, 1024, bandPass, 512, signalBuffer);
	//WriteWaveFile("bandpass.wav",rate,signalBuffer,1024);

	// filter the data to create noise, first pass
	RunFIRFilter(aweightedData, 2048 - 512, bandReject, 512, bandRejectResults);

	//WriteWaveFile("reject1.wav",rate,bandRejectResults,2048-512);

	// filter the data to create noise, second pass
	RunFIRFilter(bandRejectResults, 1024, bandReject, 512, noise);
	//WriteWaveFile("noise.wav",rate,noise,1024);

	// sum up signal and noise
	rmsSignal = 0;
	rmsNoise = 0;
	for (i = 0; i < 1024; i++) {
		rmsSignal += signalBuffer[i] * signalBuffer[i];
		rmsNoise += noise[i] * noise[i];
	}

	// finish computing rms values
	rmsSignal = sqrt((double)rmsSignal / 1024);
	rmsNoise = sqrt((double)rmsNoise / 1024);

	// check if the signal is zero
	if (rmsSignal == 0)
		return 0.0;

	// calculate THD+N
	thdn = 20.0 * log10(rmsNoise / rmsSignal);
	return thdn;

#if 0
	// calculate amplitude (magic # = (2^31)(sqrt(2)/2)(10^(-2/20)))
	result->ampL = 8.0 + 20.0 * log10(rmsSignalL / 1206187622L);
	result->ampR = 8.0 + 20.0 * log10(rmsSignalR / 1206187622L);

	// fudge the amplitude
	// fixme ... result->ampL += device->getDeviceInfo(INFO_FUDGE_AMP);
	//result->ampR += device->getDeviceInfo(INFO_FUDGE_AMP);

	// return if we don't need to compute max amplitude
	if (saveMax == false)
		return;

	// save the maximum amplitude for the left and right channels
	for (i = 0, result->maxAmpL = 0, result->maxAmpR = 0; i < 2560; i++) {
		int l, r;

		l = abs(inputDataL[i]);
		r = abs(inputDataR[i]);

		if (l > result->maxAmpL)
			result->maxAmpL = l;
		if (r > result->maxAmpR)
			result->maxAmpR = r;
	}
#endif
}

#if 0
// computeDynRange
// Compute the dynamic range given the THD+N measurement at 60 dB down.
void computeDynRange
(
	THDN_RSLT 	*thdnDynRange,
   DYN_RSLT 	*dynResult,
   double		fudge
)
{
    // calculate dynamic range
    dynResult->dynL = (-1.0 * thdnDynRange->thdnL) + 60.0;
    dynResult->dynR = (-1.0 * thdnDynRange->thdnR) + 60.0;

    // fudge the dynamic range
	dynResult->dynL += fudge;
	dynResult->dynR += fudge;
}

#endif

// AnalysisManager::verifyRate
// Verify that the given buffer contains a sine wave of the specified
// frequency at the specified sample rate. Assumes 32-bit, stereo data.
// If the waveform appears to be at the correct sample rate, return 0.
// Otherwise, return 1.
bool verifyRate(int rate,int *data,int samples,double freq)
{
	 // the action samples in a period and the zero crossings
	 int beginPeriod,endPeriod;

	 // compute the period in samples
	 double correctPeriodSamples = (double)rate / freq;
	 double actualPeriodSamples,ratio;

	 // find the last positive-to-negative zero crossing
	 for (endPeriod=samples-1;endPeriod > 0;endPeriod--) {
			if ((data[endPeriod - 1] > 0) && (data[endPeriod] < 0))
				 break;
			}

	 // make sure we found a terminating zero crossing
	 if (endPeriod == 0)
	 {
		  return false;
	 }

	 // find the next positive-to-negative zero crossing
	 for (beginPeriod=endPeriod-1;beginPeriod > 0;beginPeriod--) {
		  if ((data[beginPeriod - 1] > 0)&&(data[beginPeriod] < 0))
				break;
		  }

	 // make sure we found a zero crossing
	 if (beginPeriod == 0)
	 {
		  return false;
	 }

	 // compute the actual number of samples observed for this period
	 actualPeriodSamples = endPeriod - beginPeriod;

	 // make sure the period is close to being correct
	 ratio = actualPeriodSamples / correctPeriodSamples;
	 if ((ratio < 0.75) || (ratio > 1.25))
	 {
		  return false;
	 }

	 // everything looks good
	 return true;
}


// AnalysisManager::computeFreqResp
// Compute the frequency response of the audio data stored in buffer 'buf',
// which contains 'count' samples. We assume 32-bit stereo data.
void computeFreqResp(int *bufL,int *bufR,int count,int maxL,int maxR,FREQ_RSLT *freqRslt)
{
	 static int i,maxAmpL,maxAmpR;

	 // find the max amplitude on the left and right channels
	 for (i=0,maxAmpL=0,maxAmpR=0;i < count;i++) {
			int l,r;

			l = abs( bufL[i] );
			r = abs( bufR[i]);

		  // find the max amplitude on the left channel
		  if ( l > maxAmpL )
				maxAmpL = l;

		  // find the max amplitude on the right channel
		  if ( r > maxAmpR )
				maxAmpR = r;
		  }

	 // if a max amplitude is zero, the dyn range is zero
	 if ((maxAmpL == 0) || (maxAmpR == 0) || (maxL == 0) || (maxR == 0) )
	 {
		  freqRslt->freqL = 0;
		  freqRslt->freqR = 0;
		  return;
	}

	 // compute the frequency response
	 
	 freqRslt->freqL = fabs(20 * log10((double)maxAmpL / maxL));
	 freqRslt->freqR = fabs(20 * log10((double)maxAmpR / maxR));

	 // fudge the frequency response
	 //r->freqL += device->getDeviceInfo(INFO_FUDGE_FREQ_RESP);
	 //r->freqR += device->getDeviceInfo(INFO_FUDGE_FREQ_RESP);
}

