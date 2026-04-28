#ifndef SAMPLINGHELPER_H
#define SAMPLINGHELPER_H

#include <stdint.h>

// Uses majorPeak() to get the frequency with peak magnitude, therefore is not good fot calculating the max frequency of the signal
float getMaxSignalPeakFreq(float *vReal, float *vImg, uint32_t sampling_frequency, uint32_t sampleBuffer_N_samples, float* ret_peakMagnitude);

float getMaxSignalFrequency(float* vReal, float* vImg, uint32_t sampling_frequency, uint32_t sampleBuffer_N_samples);

#endif /* SAMPLINGHELPER_H */