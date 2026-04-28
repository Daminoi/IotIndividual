#include "myHal.h"

#include "../../include/CommonDefs.h"

uint32_t getAppropriateADCSampleFreq(uint32_t target)
{
    if(target > SAMPLING_MAX_FREQ)
        return SAMPLING_MAX_FREQ;
    
    else if(target < 650)
        return 650;
    else 
        return target;     
}

void setup_adc_continuous(uint32_t sampling_freq_chosen, uint32_t maxStoreBufSizeDMA, uint32_t nConvPerReading, adc_continuous_handle_t *handle_to_adc_driver)
{
    adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = maxStoreBufSizeDMA,
        .conv_frame_size = nConvPerReading * SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, handle_to_adc_driver));

	adc_digi_pattern_config_t adc_pattern_conf[1] = {0};
	adc_pattern_conf[0].atten 		= ADC_ATTENUATION_NEEDED;
	adc_pattern_conf[0].channel 	= ADC_CHANNEL_CHOSEN;
	adc_pattern_conf[0].unit 		= ADC_UNIT_USED;
	adc_pattern_conf[0].bit_width 	= SOC_ADC_DIGI_MAX_BITWIDTH;

    adc_continuous_config_t continuous_sampling_adc_config = {
        .pattern_num = 			1,
        .adc_pattern = 			adc_pattern_conf,
        .sample_freq_hz = 		sampling_freq_chosen,
        .conv_mode = 			ADC_CONV_MODE,
        .format = 				ADC_DIGI_OUTPUT_FORMAT_TYPE2,   // TYPE1 or TYPE2 ?????????????
    };

    ESP_ERROR_CHECK(adc_continuous_config(*handle_to_adc_driver, &continuous_sampling_adc_config));
}

esp_err_t read_dma_buffer_adc_continuous_mode(adc_continuous_handle_t* handle_to_adc_driver, uint8_t* tempBuffer, uint32_t readLength, uint32_t* bytesRead, uint32_t blockingTimeoutMS)
{
    return adc_continuous_read(*handle_to_adc_driver, tempBuffer, readLength, bytesRead, blockingTimeoutMS);
}

void start_adc_continuous_sampling(adc_continuous_handle_t* handle_to_adc_driver)
{
    ESP_ERROR_CHECK(adc_continuous_start(*handle_to_adc_driver));
}

void stop_adc_continuous_sampling(adc_continuous_handle_t* handle_to_adc_driver)
{
    ESP_ERROR_CHECK(adc_continuous_stop(*handle_to_adc_driver));
}

void deinit_adc_continuous_configuration(adc_continuous_handle_t* handle_to_adc_driver)
{
    ESP_ERROR_CHECK(adc_continuous_deinit(*handle_to_adc_driver));
}

uint32_t getMillisToFillBuffer(uint32_t nSamplesPerBuffer, uint32_t samplingFrequency)
{
    return (uint32_t) ((1000*nSamplesPerBuffer)/(samplingFrequency));
}

uint32_t getAppropriateSampleBufferSize(uint32_t samplingFrequency){
    if(samplingFrequency > 20000)
        return N_CONV_PER_READING;
    else if(samplingFrequency > 10000)
        return 2048;
    else if(samplingFrequency > 5000)
        return 1024;
    else if(samplingFrequency > 2500)
        return 512;
    else
        return 256;
}

uint32_t getAppropriateDMABufferSizeBytes(uint32_t sampleBufferSize)
{
    return sampleBufferSize * 2 * SOC_ADC_DIGI_DATA_BYTES_PER_CONV;
}

uint32_t getMillisInterval(TickType_t start, TickType_t end)
{
    return (uint32_t) (end - start) * portTICK_PERIOD_MS;
}