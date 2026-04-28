#ifndef MYHAL_H
#define MYHAL_H

#include <stdint.h>
#include <FreeRTOS.h>
#include <esp_adc/adc_continuous.h>

#define SAFETY_MARGIN_LIGHT_SLEEP_MILLIS    portTICK_PERIOD_MS * 2

uint32_t getAppropriateADCSampleFreq(uint32_t target);

uint32_t getMillisToFillBuffer(uint32_t nSamplesPerBuffer, uint32_t samplingFrequency);

uint32_t getAppropriateSampleBufferSize(uint32_t samplingFrequency);
uint32_t getAppropriateDMABufferSizeBytes(uint32_t sampleBufferSize);

void setup_adc_continuous(uint32_t sampling_freq_chosen, uint32_t maxStoreBufSizeDMA, uint32_t nConvPerReading, adc_continuous_handle_t* handle_to_adc_driver);
void deinit_adc_continuous_configuration(adc_continuous_handle_t* handle_to_adc_driver);

esp_err_t read_dma_buffer_adc_continuous_mode(adc_continuous_handle_t* handle_to_adc_driver, uint8_t* tempBuffer, uint32_t readLength, uint32_t* bytesRead, uint32_t blockingTimeoutMS);

void start_adc_continuous_sampling(adc_continuous_handle_t* handle_to_adc_driver);
void stop_adc_continuous_sampling(adc_continuous_handle_t* handle_to_adc_driver);


uint32_t getMillisInterval(TickType_t start, TickType_t end);

#endif /* MYHAL_H */