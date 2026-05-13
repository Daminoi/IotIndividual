#ifndef COMMONDEFS_H
#define COMMONDEFS_H

// uncomment the following definition if the program should communicate via LORA + TTN
#define COMMS_VIA_LORA_TTN 1

// uncomment the following definition if the program should communicate via WIFI + MQTT
//#define COMMS_VIA_WIFI_MQTT 1
// uncomment the following line of code and the previous one to do round trip time testing on the MQTT+WiFi transmission.
// The board will publish and subscribe to the same topic, measuring the time it takes from the publishing to the reception of the same message (the round trip time)
//#define ONLY_TEST_MQTT_RTT  1

// To comply with TTN rules and LoRa limitations, 60 seconds pause between LoRa transmissions
#define LORA_MINIMUM_PAUSE_BETWEEN_TRANSMISSION_MILLIS 60000
// 5 seconds pause between wifi transmissions
#define WIFI_MINIMUM_PAUSE_BETWEEN_TRANSMISSION_MILLIS 5000


// comment the following line to disable debugging via LEDs and/or serial
#define DEBUG_ACTIVE 1


// The following are all the used GPIOs

// This GPIO pin is used to sample the signal coming from the 3.5mm jack of the laptop.
#define USED_ADC_GPIO 2

// Red status LED GPIO
#define STATUS_RLED 3
// Green status LED GPIO
#define STATUS_GLED 4
// Blue status LED GPIO
#define STATUS_BLED 5

#define LORA_VEXT_GPIO  36   // HIGH to power on the LoRa module

// Definitions of adc channels, sampling buffer

#define ADC_UNIT_USED                   ADC_UNIT_1
#define ADC_CONV_MODE                   ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTENUATION_NEEDED          ADC_ATTEN_DB_11

// I cannot get my code and board to sample above this frequency correctly
#define SAMPLING_MAX_FREQ			    48000

// GPIO 2 corresponding to CHANNEL 1 of ADC1 is used for sampling
#define ADC_CHANNEL_CHOSEN              ADC_CHANNEL_1

// HAS TO BE A POWER OF 2, see below
#define	N_CONV_PER_READING				2048
#define READ_LEN                    	SOC_ADC_DIGI_DATA_BYTES_PER_CONV * N_CONV_PER_READING
// HAS TO BE A POWER OF 2, see below
#define MAX_N_OF_FULL_READINGS_SAVED	1


#define DMA_BUFFER_ADC_SIZE_BYTES       READ_LEN * 2

// THIS NUMBER HAS TO BE A POWER OF 2 to keep the FFT efficient!
#define SAMPLE_BUFFER_N_SAMPLES			N_CONV_PER_READING * MAX_N_OF_FULL_READINGS_SAVED

#define FFT_ITERATIONS                  10

#endif /* COMMONDEFS_H */