#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "fft_calc.h"
#include "esp_timer.h"
#include "secrets.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "esp_sntp.h"
#include <time.h>
#include "setup.h"

esp_mqtt_client_handle_t mqtt_client = NULL;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

#define TAG "SAMPLER_ESP32S3"

#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0

#define NUM_SAMPLES 1024
#define MAX_SAMPLE_RATE 20000.0f
#define MIN_BLOCKING_SAMPLE_RATE 100.0f
#define MIN_SAMPLE_RATE 50.0f

adc_oneshot_unit_handle_t adc_handle = NULL;

static volatile float sample_freq = MAX_SAMPLE_RATE;
static volatile float actual_freq = 0.0f;

TaskHandle_t xFFTTaskHandle = NULL;

static int read_buffer[NUM_SAMPLES];
static portMUX_TYPE g_data_mux = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    float dominant_freq;
    float max_power;
    float highest_freq;
} audio_data_t;

QueueHandle_t mqtt_queue = NULL;



void vAdaptiveAnalogSamplerTask(void *args) {
    uint64_t start_time, end_time;
    for (;;) {
        float freq = MAX_SAMPLE_RATE;
        static int temp_buf[NUM_SAMPLES];
        float sampled_freq = 0.0f;

        taskENTER_CRITICAL(&g_data_mux);
        freq = sample_freq;
        taskEXIT_CRITICAL(&g_data_mux);

        if (freq > MIN_BLOCKING_SAMPLE_RATE) {
            uint32_t delay_us = (uint32_t)(1000000.0f / freq);

            start_time = esp_timer_get_time();

            for (int i = 0; i < NUM_SAMPLES; i++) {
                int raw_value;
                ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
                temp_buf[i] = (int) raw_value;

                if (i % 300 == 0 && i != 0) {
                vTaskDelay(1); 
                } else {
                    esp_rom_delay_us(delay_us);
                }
            }

            end_time = esp_timer_get_time();
            sampled_freq = (1000000.0f * NUM_SAMPLES) / (float)(end_time - start_time);

        } else {
            TickType_t delay_ticks = pdMS_TO_TICKS((uint32_t)(1000.0f / freq));
            if (delay_ticks < 10) {
                delay_ticks = 10;
            }

            sampled_freq = 1000.0f / (float)delay_ticks;
            TickType_t xLastWakeTime = xTaskGetTickCount();

            start_time = esp_timer_get_time();
            for (int i = 0; i < NUM_SAMPLES; i++) {
                int raw_value;
                ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
                temp_buf[i] = (int) raw_value;
                
                xTaskDelayUntil(&xLastWakeTime, delay_ticks);
            }
            end_time = esp_timer_get_time();
        }
        
        ESP_LOGI(TAG, "<Sampler> Sampling frequency: %.2f Hz\nExecution time: %" PRIu64 " us\n", sampled_freq, end_time - start_time);

        taskENTER_CRITICAL(&g_data_mux);
        for (int i = 0; i < NUM_SAMPLES; i++) {
            read_buffer[i] = temp_buf[i];
        }
        actual_freq = sampled_freq;
        taskEXIT_CRITICAL(&g_data_mux);

        xTaskNotifyGive(xFFTTaskHandle);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vFastSampler(void *args) {
    uint64_t start_time = esp_timer_get_time();

    int i = 0;
    while (esp_timer_get_time() - start_time < 1000000) {
        int raw_value;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
        i++;
    }
    uint64_t execution_time = esp_timer_get_time() - start_time;

    ESP_LOGI(TAG, "<FastSampler> Completed Operation in: %" PRIu64 " us\nSample number: %d\nAverage frequency: %.2f Hz", execution_time, i, (float) i * 1000000.0f / (float) execution_time);

    uint64_t sample_time[100];
    for (int j = 0; j < 100; j++) {
        uint64_t start_sample_time = esp_timer_get_time();
        int raw_value;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value));
        sample_time[j] = esp_timer_get_time() - start_sample_time;
    }

    uint64_t avg_oneshot_time = 0;
    for (int k = 0; k < 100; k++) {
        avg_oneshot_time += sample_time[k];
    }
    avg_oneshot_time /= 100;

    ESP_LOGI(TAG, "<FastSampler> Average time per adc_oneshot_read: %" PRIu64 " us\n", avg_oneshot_time);

    for(;;) {vTaskDelay(pdMS_TO_TICKS(1000));}
}

void vFFTTask(void *args) {
    float max_power, highest_freq;
    int local_samples[NUM_SAMPLES];
    float local_sample_rate = 0.0f;

    for(;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        taskENTER_CRITICAL(&g_data_mux);
        for (int i = 0; i < NUM_SAMPLES; i++) {
            local_samples[i] = read_buffer[i];
        }
        local_sample_rate = actual_freq;
        taskEXIT_CRITICAL(&g_data_mux);

        float ret = process_fft(local_samples, NUM_SAMPLES, local_sample_rate, &max_power, &highest_freq);

        ESP_LOGI(TAG, "<FFT> Max Power on frequency: %.2f Hz\n",ret);
        ESP_LOGI(TAG, "<FFT> Highest Frequency: %.2f Hz\n",highest_freq);

        float new_sample_freq = highest_freq * 5.0f;

        if (new_sample_freq < MIN_SAMPLE_RATE) {
            new_sample_freq = MIN_SAMPLE_RATE;
        }

        if (new_sample_freq > MAX_SAMPLE_RATE) {
            new_sample_freq = MAX_SAMPLE_RATE;
        }

        taskENTER_CRITICAL(&g_data_mux);
        sample_freq = new_sample_freq;
        taskEXIT_CRITICAL(&g_data_mux);

        ESP_LOGI(TAG, "Adattamento frequenza di campionamento a: %.2f Hz\n", new_sample_freq);

        audio_data_t data_to_send = {
            .dominant_freq = ret,
            .max_power = max_power,
            .highest_freq = highest_freq
        };

        if (mqtt_queue != NULL) {
            xQueueSend(mqtt_queue, &data_to_send, pdMS_TO_TICKS(100));
        }
        
    }
}

void init_adc_oneshot() {
    // Inizializziamo l'unità ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));
    
    ESP_LOGI(TAG, "ADC OneShot Inizializzato");
}

static void sync_time(void) {
    ESP_LOGI(TAG, "Inizializzazione SNTP. Sincronizzazione orario in corso...");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org"); // Server mondiale del tempo
    esp_sntp_init();

    int retry = 0;
    const int retry_count = 15;
    
    // Aspettiamo finché l'orologio non si aggiorna (massimo 30 secondi)
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "In attesa della rete e dell'orario... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
    // Stampiamo l'ora per essere sicuri
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Orario sincronizzato con successo: %s", asctime(&timeinfo));
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) { 
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Ritento la connessione al Wi-Fi... (%d/10)", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "SUCCESSO! Indirizzo IP ottenuto: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Attendiamo che il router ci dia un IP VERO...");

    // Si blocca qui e aspetta l'IP VERO
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi connesso con successo, passiamo al cloud!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Impossibile connettersi al Wi-Fi. Fermo tutto!");
        vTaskSuspend(NULL); // Congela la scheda se il Wi-Fi non va
    }
}

static void aws_iot_mqtt_init(void) {
    // Configurazione ESP-IDF v5/v6 per MQTT Sicuro (MbedTLS)
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = AWS_IOT_ENDPOINT,
            .address.port = 8883, // La porta standard per MQTT over TLS
            .verification.certificate = (const char *)AWS_CERT_CA
        },
        .credentials = {
            .authentication = {
                .certificate = (const char *)AWS_CERT_CRT,
                .key = (const char *)AWS_CERT_PRIVATE
            }
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "Client MQTT AWS avviato!");
}

void vMQTTPublishTask(void *args) {
    audio_data_t data_to_send;
    char json_payload[128];

    // Aspettiamo un attimo che il Wi-Fi e MQTT si assestino
    vTaskDelay(pdMS_TO_TICKS(3000));

    for(;;) {
        // Aspetta senza consumare CPU finché la FFT non produce un nuovo risultato
        if (xQueueReceive(mqtt_queue, &data_to_send, portMAX_DELAY) == pdTRUE) {
            
            // Creiamo il pacchetto JSON (uguale a quello che faresti in Arduino)
            snprintf(json_payload, sizeof(json_payload), 
                     "{\"dom_freq\":%.2f, \"high_freq\":%.2f, \"power\":%.0f}", 
                     data_to_send.dominant_freq, data_to_send.highest_freq, data_to_send.max_power);
            
            // Invio ad AWS IoT Core (Topic: esp32/pub, QoS: 0, Retain: 0)
            if (mqtt_client != NULL) {
                esp_mqtt_client_publish(mqtt_client, "esp32/pub", json_payload, 0, 0, 0);
                ESP_LOGI("AWS_MQTT", "Pubblicato: %s", json_payload);
            }
        }
    }
}

void app_main(void) {
    BaseType_t ret;

    init_adc_oneshot();

    if (MODE == DEMO) {
        esp_err_t err_ret = nvs_flash_init();
        if (err_ret == ESP_ERR_NVS_NO_FREE_PAGES || err_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err_ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err_ret);
        
        init_fft(NUM_SAMPLES);

        wifi_init_sta();
        sync_time();
        aws_iot_mqtt_init();

        mqtt_queue = xQueueCreate(10, sizeof(audio_data_t));

        ret = xTaskCreatePinnedToCore(vAdaptiveAnalogSamplerTask, "Sampler", 4096, NULL, 5, NULL, 1);

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "SETUP: <Impossibile creare il task Sampler>\n");
        }

        ret = xTaskCreatePinnedToCore(vFFTTask, "FFT", 8192, NULL, 5, &xFFTTaskHandle, 0);

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "SETUP: <Impossibile creare il task FFT>\n");
        }

        ret = xTaskCreatePinnedToCore(vMQTTPublishTask, "MQTTPublish", 4096, NULL, 5, NULL, 0);

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "SETUP: <Impossibile creare il task MQTTPublish>\n");
        }
    }

    else if (MODE == FAST_SAMPLER) {
        ESP_LOGI(TAG, "<SETUP>: Starting FastSampler Task for testing ADC performance");

        ret = xTaskCreatePinnedToCore(vFastSampler, "FastSampler", 4096, NULL, 1, NULL, 1);

        if (ret != pdPASS) {
            ESP_LOGE(TAG, "SETUP: <Impossibile creare il task FastSampler>\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}