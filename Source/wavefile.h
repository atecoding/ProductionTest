class ProductionUnit;

void WriteWaveFile(ProductionUnit* unit, String filename,int rate,AudioSampleBuffer *asb);
void WriteWaveFile(ProductionUnit* unit, String filename, int rate, double *data, int samples);