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
    
    if (nullptr == stream || samples < 1)
        return;
    
	writer = waf.createWriterFor(stream,rate,asb->getNumChannels(),32,meta,0);
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
    if (samples < 1)
        return;
    
	int i;
	AudioSampleBuffer asb(1,samples);
	float *dest = asb.getWritePointer(0);
    
    if (nullptr == dest)
        return;

	for (i = 0; i < samples; i++)
		dest[i] = (float) data[i];

	WriteWaveFile(unit, filename,rate, &asb, samples);
}

void WriteWaveFile(String filename, int rate, AudioSampleBuffer *asb, int samples)
{
    if (samples < 1)
        return;
    
    File folder(File::getCurrentWorkingDirectory());
    File f;
    FileOutputStream *stream;
    WavAudioFormat waf;
    AudioFormatWriter *writer;
    StringPairArray meta;
    
    f = folder.getChildFile(filename);
    f.deleteFile();
    
    stream = f.createOutputStream();
    
    if (nullptr == stream)
        return;
    
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
    if (samples < 1)
        return;
    
	AudioSampleBuffer asb(1, samples);

	float *write = asb.getWritePointer(0);
    
    if (nullptr == write)
        return;
    
	for (int i = 0; i < samples; ++i)
	{
		write[i] = (float)buffer[i];
	}

	WriteWaveFile(filename, rate, &asb, samples);
}

void WriteWaveFile(File wavefile, int rate, AudioSampleBuffer *asb, int samples, BigInteger channels)
{
    if (samples < 1)
        return;
    
    int numDestinationChannels = channels.countNumberOfSetBits();
    AudioSampleBuffer destinationBuffer(numDestinationChannels, samples);
    
    int sourceChannel = 0;
    for (int destinationChannel = 0; destinationChannel < numDestinationChannels; ++destinationChannel)
    {
        sourceChannel = channels.findNextSetBit(sourceChannel);
        if (sourceChannel < 0)
            return;
        destinationBuffer.copyFrom(destinationChannel, 0, *asb, sourceChannel, 0, samples);
        sourceChannel++;
    }
    
    wavefile.deleteFile();
    FileOutputStream* stream = wavefile.createOutputStream();
    if (stream)
    {
        WavAudioFormat waf;
        StringPairArray meta;
        AudioFormatWriter* writer = waf.createWriterFor(stream,rate,numDestinationChannels,32,meta,0);
        if (writer)
        {
            writer->writeFromAudioSampleBuffer(destinationBuffer, 0, samples);
            delete writer;
        }
        else
        {
            delete stream;
        }
    }
}

#endif
