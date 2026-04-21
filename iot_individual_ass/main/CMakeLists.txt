idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES fft esp-dsp esp_adc esp_timer nvs_flash esp_wifi esp_netif mqtt)