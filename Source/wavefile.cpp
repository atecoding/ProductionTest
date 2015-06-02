#include "base.h"
#include "ProductionUnit.h"

#if WRITE_WAVE_FILES

void WriteWaveFile(ProductionUnit* unit, String filename, int rate, AudioSampleBuffer *asb, int samples)
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
		writer->writeFromAudioSampleBuffer(*asb, 0, samples);
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

	WriteWaveFile(unit, filename,rate, &asb, samples);
}

void WriteWaveFile(String filename, int rate, AudioSampleBuffer *asb, int samples)
{
    File folder(File::getCurrentWorkingDirectory());
    File f;
    FileOutputStream *stream;
    WavAudioFormat waf;
    AudioFormatWriter *writer;
    StringPairArray meta;
    
    f = folder.getChildFile(filename);
    f.deleteFile();
    
    stream = f.createOutputStream();
    writer = waf.createWriterFor(stream,rate,1,32,meta,0);
    if (writer)
    {
        writer->writeFromAudioSampleBuffer(*asb, 0, samples);
        delete writer;
    }
    else
    {
        delete stream;
    }
}

void WriteWaveFile(String filename, int rate, double* buffer, int samples)
{
	AudioSampleBuffer asb(1, samples);

	float *write = asb.getWritePointer(0);
	for (int i = 0; i < samples; ++i)
	{
		write[i] = (float)buffer[i];
	}

	WriteWaveFile(filename, rate, &asb, samples);
}

#endif
