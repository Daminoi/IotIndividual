#include <Arduino.h>
#include <FreeRTOS.h>
#include <esp_adc/adc_continuous.h>
#include <esp_timer.h>

#include "../include/CommonDefs.h"
#include "../include/credentialsHardcoded.h"

#include <WiFi.h>
#include <esp-mqtt-arduino.h>

#include <heltec_unofficial.h>
#include <RadioLib.h>


#include "../lib/debugFriend/debugFriend.h"
#include "../lib/samplingHelper/samplingHelper.h"
#include "../lib/myHal/myHal.h"

adc_continuous_handle_t handle_to_adc_driver = NULL;

float vReal[SAMPLE_BUFFER_N_SAMPLES];
float vImag[SAMPLE_BUFFER_N_SAMPLES] = {0};
uint8_t temp_buffer[READ_LEN] = {0};


// Depending on the sampling frequency, the buffer size may be reduced to reduce the latency
uint32_t sampleBufferNewSize; 

uint32_t detectedMaximumFrequency;
uint32_t optimalSamplingFrequency;
uint32_t chosenSamplingFrequency;

uint32_t millisToFillReadBuffer;


QueueHandle_t 		promptTransmissionTask;
uint8_t				transmissionTaskState;
// spinlock
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;


Mqtt5ClientESP32 mqttClient;
volatile bool mqttReady = false;

LoRaWANNode* thisNode;
uint8_t subBandSetting;
LoRaWANBand_t regionEurope;
int16_t SX1262RadioState;
uint8_t networkKey[16];

void ReadAndFFTTask(void* pvParameters);
void ReadDMAAndComputeTask(void* pvParameters);
void TransmissionTask(void* pvParameters);

void MqttRttTestTask(void* pvParameters);

void shared_vars_setup()
{
	promptTransmissionTask 			= xQueueCreate(1, 16);
	transmissionTaskState			= 0;
}

void setup()
{
	// radio variable is defined by heltec_unofficial library
	heltec_setup();
	
	pinMode(8, OUTPUT);
	pinMode(STATUS_RLED, OUTPUT);
	pinMode(STATUS_GLED, OUTPUT);
	pinMode(STATUS_BLED, OUTPUT);
	
	#ifdef DEBUG_ACTIVE
    Serial.begin(115200);
	Serial.println("Welcome!");
	#endif
	
	shared_vars_setup();
	
	xTaskCreatePinnedToCore(
		ReadAndFFTTask, 
		"Reader_From_DMA_ADC_and_FFT", 
		4096, 
		NULL, 
		2, 
		NULL, 
		1
	);
}

void loop()
{
	vTaskDelete(NULL);
}

/* 	
The following task runs only at the beginning, calculates the optimal sampling frequency and then creates:
- the ReadDMAAndComputeTask
- the transmissionTask

that will run on separates cores (the ESP32S3 has 2 identical Extensa LX7 cores).
*/
void ReadAndFFTTask(void* pvParameters)
{
	setup_adc_continuous(SAMPLING_MAX_FREQ, DMA_BUFFER_ADC_SIZE_BYTES, N_CONV_PER_READING, &handle_to_adc_driver);
	
	start_adc_continuous_sampling(&handle_to_adc_driver);
	
	
	#ifdef DEBUG_ACTIVE
	setLEDStatusBLUE();
	#endif
	
	
	uint32_t sampleBufferIndex = 0;
    // The number of values read with the following operation from the memory where the adc has stored them
	uint32_t bytes_read;
	esp_err_t read_result;
	
	float tempSumSamplingFreq 	= 0;
	float nFFTCalculated 		= 0;
	
	while(1)
	{
		read_result = read_dma_buffer_adc_continuous_mode(&handle_to_adc_driver, temp_buffer, READ_LEN, &bytes_read, 0);
		
		/*
		#ifdef DEBUG_ACTIVE
		if(read_result == ESP_ERR_INVALID_STATE || read_result == ESP_ERR_TIMEOUT){
			Serial.println("\nERR adc too fast or operation timed out!");
		}
		#endif
		*/

		// Sampled data is saved to the buffer to perform the FFT
		if (read_result == ESP_OK)
		{
			#ifdef DEBUG_ACTIVE
			//Serial.printf("\nData from the adc: OK! (%u bytes read)\n", bytes_read);
			#endif
	
			for (uint32_t i = 0; i < bytes_read && sampleBufferIndex < SAMPLE_BUFFER_N_SAMPLES; i += SOC_ADC_DIGI_RESULT_BYTES)
			{
				vReal[sampleBufferIndex] = ( (adc_digi_output_data_t *)&temp_buffer[i] )-> type2.data;
				
				sampleBufferIndex += 1;
			}
			sampleBufferIndex = 0;

			memset(vImag, 0, sizeof(float)*SAMPLE_BUFFER_N_SAMPLES);

			tempSumSamplingFreq += getMaxSignalFrequency(vReal, vImag, SAMPLING_MAX_FREQ, SAMPLE_BUFFER_N_SAMPLES);
			nFFTCalculated++;


			if(nFFTCalculated >= FFT_ITERATIONS){
				stop_adc_continuous_sampling(&handle_to_adc_driver);
				deinit_adc_continuous_configuration(&handle_to_adc_driver);

				detectedMaximumFrequency 	= (uint32_t) tempSumSamplingFreq / nFFTCalculated;
				optimalSamplingFrequency 	= detectedMaximumFrequency * 2.1;
				
				#ifdef TESTING_KEEP_MAX_SAMPLING_FREQUENCY
				chosenSamplingFrequency 	= SAMPLING_MAX_FREQ;
				#else
				chosenSamplingFrequency 	= getAppropriateADCSampleFreq(optimalSamplingFrequency);
				#endif

				sampleBufferNewSize = getAppropriateSampleBufferSize(chosenSamplingFrequency);
				
				millisToFillReadBuffer = getMillisToFillBuffer(sampleBufferNewSize, chosenSamplingFrequency);

				Serial.printf("Maximum frequency found: %u Hz\nOptimal frequency: %u Hz\nChosen frequency ADC DMA: %u Hz\n", detectedMaximumFrequency, optimalSamplingFrequency, chosenSamplingFrequency);
				Serial.printf("Sample buffer new size (num samples): %u\nMilliseconds to fill buffer: %u ms\n", sampleBufferNewSize, millisToFillReadBuffer);

				#ifdef COMMS_VIA_WIFI_MQTT
				WiFi.begin(CH_SSID_WIFI, CH_PASSWORD_WIFI);
				while(WiFi.status() != WL_CONNECTED){
					Serial.println("Trying to connect to WiFi");
					vTaskDelay(500 * portTICK_PERIOD_MS);
				}
				Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
				
				
				mqttClient.onConnected([]{
					mqttReady = true;
					
					#ifdef ONLY_TEST_MQTT_RTT
    				bool ok = mqttClient.subscribe(RTT_TESTING_TOPIC_PONG, 0);
					
    				Serial.printf("Subscribe result: %s\n", ok ? "OK" : "FAILED");
					#endif
				});
				
				mqttClient.onDisconnected([]{
					mqttReady = false;
				});
				
				#ifdef ONLY_TEST_MQTT_RTT
				mqttClient.onMessage([](const char* topic, size_t topic_len, const uint8_t* data, size_t len){
					uint64_t timeReceive = esp_timer_get_time();
					Serial.printf("Message read back, time since start in us: %d\n\n", timeReceive);
					
					/*if(strncmp(topic, RTT_TESTING_TOPIC_PONG, sizeof(RTT_TESTING_TOPIC_PONG)) == 0)
					{
						ticksPublish = *(uint32_t*)data;
						
						Serial.printf("Ticks before publish: %d; message read back now, ticks: %d\n", ticksPublish, xTaskGetTickCount());
						}*/
				});
				#endif
				
				mqttClient.begin(CH_BROKER_FULL, CH_CLIENT_NAME);
				
				mqttClient.connect();
				#endif

				#ifdef COMMS_VIA_LORA_TTN


				subBandSetting = 0;
				regionEurope = EU868;
				thisNode = new LoRaWANNode(&radio, &regionEurope, subBandSetting);

				/*
				SX1262RadioState = radio.begin();
				if(SX1262RadioState != RADIOLIB_ERR_NONE)
				{
    				Serial.printf("Radio begin() FAILED with state: %u\n", SX1262RadioState);
    				unrecoverableErrorStatus();
  				}
				*/

				// On TTN, my Heltec V3.2 is registered as LoRaWan 1.0.2 capable. The function beginABP() supports LoRaWan 1.1 too,
				// so to make it work with LoRaWan 1.0.2 it only requires to pass the same NWKSKEY as fNwkSIntKey, sNwkSIntKey, nwkSEncKey.
				SX1262RadioState = (*thisNode).beginABP(CH_DEVADDR, CH_NWKSKEY, CH_NWKSKEY, CH_NWKSKEY, CH_APPSKEY);
  				if(SX1262RadioState != RADIOLIB_ERR_NONE)
				{
    				Serial.printf("ABP activation FAILED: %d\n", SX1262RadioState);
					unrecoverableErrorStatus();
  				}

				SX1262RadioState = (*thisNode).activateABP();
				if(SX1262RadioState == RADIOLIB_LORAWAN_NEW_SESSION || SX1262RadioState == RADIOLIB_LORAWAN_SESSION_RESTORED)
				{
    				Serial.println("ABP join successfull");
  				}
				else
				{
    				Serial.printf("ABP join FAILED with state: %d\n", SX1262RadioState);
					unrecoverableErrorStatus();
  				}
				#endif


				setLEDStatusOFF();

				setLEDStatusGREEN();
				delay(2000);
				setLEDStatusOFF();

				#ifdef ONLY_TEST_MQTT_RTT
				
				xTaskCreatePinnedToCore(
					MqttRttTestTask,
					"Test_MQTT_RTT_WiFi",
					4096,
					NULL,
					2,
					NULL,
					0
				);

				#else
				
				xTaskCreatePinnedToCore(
					ReadDMAAndComputeTask, 
					"Read_DMA_ADC_and_compute_aggregate_value", 
					4096, 
					NULL, 
					2, 
					NULL, 
					1
				);

				xTaskCreatePinnedToCore(
					TransmissionTask, 
					"Transmit_via_LoRa_or_WiFi", 
					8192, 
					NULL, 
					2, 
					NULL, 
					0
				);
				#endif

				vTaskDelete(NULL);	// This task terminates, as its job (calculating the optimal sampling frequency and starting the other threads) is completed
			}
		}
	}
}


void ReadDMAAndComputeTask(void* pvParameters)
{
	setup_adc_continuous(chosenSamplingFrequency, getAppropriateDMABufferSizeBytes(sampleBufferNewSize), sampleBufferNewSize, &handle_to_adc_driver);
	
	start_adc_continuous_sampling(&handle_to_adc_driver);

	setLEDStatusGREEN();

	uint32_t sampleBufferIndex = 0;
    // The number of values read with the following operation from the memory where the adc has stored them
	uint32_t bytes_read;
	esp_err_t read_result;

	uint64_t sumReadings = 0;
	uint32_t nReadings   = 0;

	TickType_t startTimeWindow = xTaskGetTickCount();

	while(1)
	{
		read_result = read_dma_buffer_adc_continuous_mode(&handle_to_adc_driver, temp_buffer, sampleBufferNewSize*SOC_ADC_DIGI_DATA_BYTES_PER_CONV, &bytes_read, 0);

		if(read_result == ESP_OK)
		{
			
			//Serial.printf("bytes read %u, expected %u\n", bytes_read, sampleBufferNewSize*SOC_ADC_DIGI_DATA_BYTES_PER_CONV);

			for (uint32_t i = 0; i < bytes_read; i += SOC_ADC_DIGI_RESULT_BYTES)
			{
				sumReadings += ( (adc_digi_output_data_t *)&temp_buffer[i] )-> type2.data;
				nReadings++;
			}

			//Serial.printf("time since last avg value sent to transmission task: %u ms\n", getMillisInterval(startTimeWindow, xTaskGetTickCount()));

			if(
				#ifdef COMMS_VIA_LORA_TTN
				getMillisInterval(startTimeWindow, xTaskGetTickCount()) > LORA_MINIMUM_PAUSE_BETWEEN_TRANSMISSION_MILLIS
				#else
					#ifdef COMMS_VIA_WIFI_MQTT
					getMillisInterval(startTimeWindow, xTaskGetTickCount()) > WIFI_MINIMUM_PAUSE_BETWEEN_TRANSMISSION_MILLIS
					#else
					getMillisInterval(startTime, xTaskGetTickCount()) > 5000
					#endif
				#endif
			)
			{
				uint16_t averagedReading = (uint16_t) (sumReadings / (uint64_t)nReadings);
				sumReadings = 0;
				
				Serial.printf("\nR&Ctask: sent on queue avg value after %u readings, time interval %u\n", nReadings, getMillisInterval(startTimeWindow, xTaskGetTickCount()));

				#ifdef COMMS_VIA_LORA_TTN
				xQueueSend(promptTransmissionTask, &averagedReading, portMAX_DELAY);
				#else
				#ifdef COMMS_VIA_WIFI_MQTT
				xQueueSend(promptTransmissionTask, &averagedReading, portMAX_DELAY);
				#else
				Serial.printf("R&Ctask: [WARNING] LoRa and WiFi NOT DEFINED on the CommonDefs.h file, this is only for testing purposes!");
				// average value is just consumed, this is only for testing purposes
				#endif
				#endif
				taskENTER_CRITICAL(&spinlock);
				transmissionTaskState = 1;
				taskEXIT_CRITICAL(&spinlock);
				

				startTimeWindow = xTaskGetTickCount();
				nReadings = 0;
			}
		}
		else if(read_result == ESP_ERR_TIMEOUT)
		{
			// We can go into light sleep, the DMA will keep sampling and filling its buffer.
			// We need to consider the transmission task; if it is transmitting, then we cannot go into light sleep!
			uint32_t canSleep;
			TickType_t wastedTime = xTaskGetTickCount();

			taskENTER_CRITICAL(&spinlock);
			if(transmissionTaskState == 0)
				canSleep = 1;
			else
				canSleep = 0;
			taskEXIT_CRITICAL(&spinlock);

			// calculate how many milliseconds the esp32s3 can light sleep
			// time to fill a READLEN buffer	-	time wasted to get to this point	-	safety_margin
			if(canSleep == 1){
				canSleep = millisToFillReadBuffer - getMillisInterval(wastedTime, xTaskGetTickCount()) - SAFETY_MARGIN_LIGHT_SLEEP_MILLIS;
				
				//esp_sleep_enable_timer_wakeup(canSleep * 1000);
				//Serial.printf("R&Ctask: going for light Sleep for %u ms\n", canSleep);
				setLEDStatusRED();
				//esp_light_sleep_start();
				vTaskDelay(pdMS_TO_TICKS(canSleep));
				setLEDStatusGREEN();
			}
		}
	}
}

void TransmissionTask(void* pvParameters)
{
	uint16_t payload;
	uint32_t messageCounter = 0;

	struct MQTTmsg {
  		int32_t payload;
  		int32_t msgCnt;
	};
	MQTTmsg newMessage;

	uint8_t loraMsg[8];

	TickType_t lastSend = xTaskGetTickCount();

	// SET UP EITHER LORA+TTN OR WIFI+MQTT

	while(1){
		xQueueReceive(promptTransmissionTask, &payload, portMAX_DELAY);

		#ifdef COMMS_VIA_WIFI_MQTT
		newMessage.payload = (uint32_t) payload;
		newMessage.msgCnt  = messageCounter;

		if(mqttReady == false)
		{
			mqttClient.connect();
			vTaskDelay(5 * portTICK_PERIOD_MS);
		}
		mqttClient.publish(RTT_TESTING_TOPIC_PING, (uint8_t*)&newMessage, sizeof(MQTTmsg));
		#else
		#ifdef COMMS_VIA_LORA_TTN

    	loraMsg[0] = 1;
    	loraMsg[1] = 0;
    	loraMsg[2] = 0;
    	loraMsg[3] = 0;
    	loraMsg[4] = 0;
    	loraMsg[5] = 0;
		loraMsg[6] = 0;
		loraMsg[7] = 0;

    	//SX1262RadioState = (*thisNode).sendReceive(loraMsg, sizeof(loraMsg));
		setLEDStatusBLUE();
    	SX1262RadioState = (*thisNode).sendReceive(loraMsg, sizeof(loraMsg), 0, NULL, NULL, true);
    	if(SX1262RadioState < 0)
		{
      		Serial.printf("LoRa tx FAILED with state: %d\n", SX1262RadioState);
    	}
		else if(SX1262RadioState == 0)
		{
			Serial.println("LoRa tx successful, no rx received");
		} 
		else
		{
			Serial.println("LoRa rx!");	// I used 0 as rx window, so no messages will be received
    	}
		#else

		#endif
		#endif

		messageCounter++;

		taskENTER_CRITICAL(&spinlock);
		setLEDStatusOFF();
		transmissionTaskState = 0;
		taskEXIT_CRITICAL(&spinlock);
	}
}

void MqttRttTestTask(void* pvParameters)
{
	uint64_t timePublish;

	while(1){

		if(mqttReady == false)
		{
			mqttClient.connect();
			vTaskDelay(5 * portTICK_PERIOD_MS);
		}
		
		timePublish = esp_timer_get_time();
		
		mqttClient.publish(RTT_TESTING_TOPIC_PING, (uint8_t*)&timePublish, sizeof(timePublish));

		Serial.printf("Just published, time since start in us:    %d\n", timePublish);


		vTaskDelay(3000 * portTICK_PERIOD_MS);
	}
}