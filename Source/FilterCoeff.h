#define FILTER_TAPS			512
#define NUM_FILTER_RATES	3

#define FILTER_INDEX_44100	0
#define FILTER_INDEX_48000	1
#define FILTER_INDEX_96000	2

extern const double A_Weight[NUM_FILTER_RATES][FILTER_TAPS];
extern const double Bandpass_1kHz[NUM_FILTER_RATES][FILTER_TAPS];
extern const double BandRjct_1kHz[NUM_FILTER_RATES][FILTER_TAPS];
extern const double Bandpass_7200Hz[FILTER_TAPS];
extern const double BandRjct_7200Hz[FILTER_TAPS];

