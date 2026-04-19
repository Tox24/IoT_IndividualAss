#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
//#include "uart_data.h"
#include "fft_calc.h"

#define TAG "SAMPLER_ESP32S3"

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0

#define NUM_SAMPLES 1024
#define MAX_SAMPLE_RATE 44100.0f
#define SAMPLE_RATE MAX_SAMPLE_RATE
#define MIN_BLOCKING_SAMPLE_RATE 1000.0f

adc_oneshot_unit_handle_t adc_handle = NULL;

static float sample_freq = 50.0f; //MAX_SAMPLE_RATE;
static float max_freq = 0.0f;

TaskHandle_t xFFTTaskHandle = NULL;

//static float fft_input[NUM_SAMPLES];
static int read_buffer[NUM_SAMPLES];


void vAdaptiveAnalogSamplerTask(void *args) {
    for (;;) {
        float freq = sample_freq > 1000.0f ? MIN_BLOCKING_SAMPLE_RATE : sample_freq;
        static int temp_buf[NUM_SAMPLES];
        if (freq > MIN_BLOCKING_SAMPLE_RATE) {
            //TODO usare esp_rom_blocking_delay_us() per campionare a frequenze più alte senza bloccare il task
        } else {
            int delay_ticks = (int) 1000 / freq; // Adaptive delay
            TickType_t xLastWakeTime = xTaskGetTickCount();

            for (int i = 0; i < NUM_SAMPLES; i++) {
                int raw_value;
                ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
                temp_buf[i] = (int) raw_value;

                printf(">signal: %d\n", temp_buf[i]); //just for debugging
                
                xTaskDelayUntil(&xLastWakeTime, delay_ticks);
            }
        }

        for (int i = 0; i < NUM_SAMPLES; i++) {
            read_buffer[i] = temp_buf[i];
        }

        xTaskNotifyGive(xFFTTaskHandle);
    }
}

void vFFTTask(void *args) {
    float max_power, highest_freq;

    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        float ret = process_fft(read_buffer, NUM_SAMPLES, sample_freq, &max_power, &highest_freq);

        ESP_LOGI(TAG, "<FFT> Max Power on frequency: %.2f Hz\n",ret);
        ESP_LOGI(TAG, "<FFT> Highest Frequency: %.2f Hz\n",highest_freq);

        printf(">frequency: %.2f\n", ret);

        if (highest_freq > max_freq) {
            max_freq = highest_freq;
            sample_freq = (max_freq * 2) + 1.0f;
        }
        
    }
}

/*
void init_adc_dma() {
    // 1. Creiamo il "secchio" gigante nella RAM
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 8192,
        .conv_frame_size = NUM_SAMPLES,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

    // 2. Configuriamo il "metronomo" (Pattern)
    adc_digi_pattern_config_t adc_pattern[1] = {0};
    adc_pattern[0].atten = ADC_ATTEN_DB_12; // Range 0 - 3.1V (Perfetto per i tuoi 1.65V)
    adc_pattern[0].channel = ADC_CHANNEL;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH; // Massima risoluzione (12 bit)

    // 3. Attiviamo la lettura continua a 44100 Hz
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2, // Formato dati specifico per ESP32-S3
        .pattern_num = 1,
        .adc_pattern = adc_pattern,
    };
    
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    
    ESP_LOGI(TAG, "ADC DMA Inizializzato a %d Hz sul Canale %d", SAMPLE_RATE, ADC_CHANNEL);
}
*/

void init_adc_oneshot() {
    // Inizializziamo l'unità ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // Configuriamo il singolo canale (GPIO 1)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // 4096 valori possibili
        .atten = ADC_ATTEN_DB_12,    // Range fino a ~3.1V (Perfetto per il tuo 1.65V)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
    
    ESP_LOGI(TAG, "ADC OneShot Inizializzato");
}

void app_main(void) {
    //init_uart();
    init_adc_oneshot();
    init_fft(NUM_SAMPLES);

    /* xAnalogValue = xQueueCreate(1, sizeof(int16_t)); // A queue that will simulate the current analog value
    if (xAnalogValue == NULL) {
        ESP_LOGE(TAG, "SETUP: failed to create queue for analog values\n");
        
        for(;;) {
            vTaskDelay(portMAX_DELAY);
        }
    }*/

    BaseType_t ret;

    ret = xTaskCreatePinnedToCore(vAdaptiveAnalogSamplerTask, "Sampler", 4096, NULL, 5, NULL, 1);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "SETUP: <Impossibile creare il task Sampler>\n");
    }

    ret = xTaskCreatePinnedToCore(vFFTTask, "FFT", 8192, NULL, 5, &xFFTTaskHandle, 1);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "SETUP: <Impossibile creare il task FFT>\n");
    }

}