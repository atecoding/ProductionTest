class ProductionUnit;

void WriteWaveFile(ProductionUnit* unit, String filename, int rate, AudioSampleBuffer *asb, int samples);
void WriteWaveFile(ProductionUnit* unit, String filename, int rate, double *data, int samples);
void WriteWaveFile(String filename, int rate, AudioSampleBuffer *asb, int samples);
void WriteWaveFile(File wavefile, int rate, AudioSampleBuffer *asb, int samples, BigInteger channels);
void WriteWaveFile(String filename, int rate, double* buffer, int samples);

