#include "base.h"
#include "ProductionUnit.h"
#if WRITE_WAVE_FILES
void WriteWaveFile(ProductionUnit* unit, String filename, int rate, AudioSampleBuffer *asb)
{
	File folder(unit->getOutputFolder());
	File f;
	FileOutputStream *stream;
	WavAudioFormat waf;
	AudioFormatWriter *writer;
	StringPairArray meta;

	filename = unit->getSerialNumber() + "-" + filename;

	f = folder.getChildFile(filename);
	f.deleteFile();

	stream = f.createOutputStream();
	writer = waf.createWriterFor(stream,rate,1,32,meta,0);
	if (writer)
	{
		writer->writeFromAudioSampleBuffer(*asb, 0, asb->getNumSamples());
		delete writer;
	}
	else
	{
		delete stream;
	}
}

void WriteWaveFile(ProductionUnit* unit, String filename, int rate, double *data, int samples)
{
	int i;
	AudioSampleBuffer asb(1,samples);
	float *dest = asb.getWritePointer(0);

	for (i = 0; i < samples; i++)
		dest[i] = (float) data[i];

	WriteWaveFile(unit, filename,rate,&asb);
}

#endif