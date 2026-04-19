#include "uart_data.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_NUM UART_NUM_0
#define BAUD_RATE 2000000

static const char *TAG = "UART_LIB";

void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // Corretto ESP_ERROR_CHECK
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUFFER_SIZE * 2, 0, 0, NULL, 0));
    
    ESP_LOGI(TAG, "Hardware UART inizializzato a 2 Mbps.");
}

int read_clean_from_uart(int16_t *values) {
    // Leggiamo i dati versandoli direttamente nel puntatore a 16 bit
    int len = uart_read_bytes(UART_NUM, values, BUFFER_SIZE, pdMS_TO_TICKS(100));

    if (len > 0) {
        return len / 2; // Restituiamo il numero di CAMPIONI (i byte sono il doppio)
    } else {
        return -1; // Ritorna -1 se c'è un timeout o non arrivano dati
    }
}