//
// Analysis.h - audio quality mathematical analysis functions
//

// type:   DYN_RSLT
// usage:  This contains the results of a dynamic range computation.
typedef struct {
    double dynL;
    double dynR;
	 } DYN_RSLT;

// type:   FREQ_RSLT
// usage:  This contains the results of a frequency response computation.
typedef struct {
    double freqL;
    double freqR;
	 } FREQ_RSLT;

double computeTHDN(float *input,int rate);

#if 0
void computeDynRange(THDN_RSLT *thdnDynRange,
							DYN_RSLT *dynResult,
							double fudge);

bool verifyRate(int rate,int *data,int samples,double freq);

void computeFreqResp(int *bufL,int *bufR,int count,int maxL,int maxR,FREQ_RSLT *r);
#endif

#define THDN_SAMPLES_REQUIRED	9600 //2560