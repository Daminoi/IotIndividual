#include "samplingHelper.h"

#include <arduinoFFT.h>

#include "../../include/CommonDefs.h"

float getMaxSignalPeakFreq(float *vReal, float *vImg, uint32_t sampling_frequency, uint32_t sampleBuffer_N_samples, float* ret_peakMagnitude)
{
    ArduinoFFT<float> fft = ArduinoFFT<float>(vReal, vImg, sampleBuffer_N_samples, sampling_frequency);
    
    fft.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    fft.compute(FFT_FORWARD);
    fft.complexToMagnitude();
    
    vReal[0] = 0;
	vReal[1] = 0;

    float peak_magn_frequency;

	fft.majorPeak(&peak_magn_frequency, ret_peakMagnitude);
    
    return peak_magn_frequency;
}


float getMaxSignalFrequency(float *vReal, float *vImg, uint32_t sampling_frequency, uint32_t sampleBuffer_N_samples)
{
    float maxFrequencyDetected = 0;

    ArduinoFFT<float> fft = ArduinoFFT<float>(vReal, vImg, sampleBuffer_N_samples, sampling_frequency);
    
    fft.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    fft.compute(FFT_FORWARD);
    fft.complexToMagnitude();
    
    vReal[0] = 0;

    float fftBinWidth = sampling_frequency / sampleBuffer_N_samples;

    // I get the standard deviation and set the threshold for a valid signal
    float mean = 0;
    float M2 = 0;
    int n = 0;
    float delta;

    for(uint32_t i = 1; i < sampleBuffer_N_samples/2; i++)
    {
        n++;
        delta = vReal[i] - mean;
        mean += delta / n;
        M2 += delta * (vReal[i] - mean);
    }

    float variance = M2 / (n - 1);
    float stddev = sqrt(variance);

    float threshold = mean + 2 * stddev;

    for (int i = 1; i < sampleBuffer_N_samples/2; i++) {
      if (vReal[i] > threshold) {
        maxFrequencyDetected = i * fftBinWidth;
      }
    }
    
    return maxFrequencyDetected;
}
