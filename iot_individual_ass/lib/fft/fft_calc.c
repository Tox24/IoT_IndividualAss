#include "fft_calc.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "FFT_LIB";

static float *fft_data;
static float *hanning_window;

void init_fft(int FFT_SAMPLES) {
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Errore inizializzazione FFT");
    }
    hanning_window = (float *)malloc(FFT_SAMPLES * sizeof(float));
    if (!hanning_window) {
        ESP_LOGE(TAG, "Errore nell'allocazione della finestra di Hanning");
        return;
    }
    dsps_wind_hann_f32(hanning_window, FFT_SAMPLES);
    ESP_LOGI(TAG, "Modulo FFT pronto (%d campioni)", FFT_SAMPLES);
}


float process_fft(int *samples,  int FFT_SAMPLES, float current_sample_rate, float *max_power, float *highest_freq) {
    // 1. Preparazione dei dati: Applichiamo la finestra di Hanning e convertiamo in float
    fft_data = (float *)malloc(FFT_SAMPLES * 2 * sizeof(float));
    if (!fft_data) {
        ESP_LOGE(TAG, "Errore nell'allocazione dei dati FFT");
        return -1.0f;
    }

    for (int i = 0; i < FFT_SAMPLES; i++) {
        fft_data[i * 2 + 0] = (float)samples[i] * hanning_window[i];
        fft_data[i * 2 + 1] = 0.0f;
    }
    
    dsps_fft2r_fc32(fft_data, FFT_SAMPLES);
    dsps_bit_rev_fc32(fft_data, FFT_SAMPLES);
    dsps_cplx2reC_fc32(fft_data, FFT_SAMPLES);

    // 3. Ricerca del Picco E della Frequenza Massima
    float power = 0;
    int peak_index = 0;
    int highest_index = 0;
    
    // SOGLIA DI RUMORE: i valori di potenza sotto questo numero sono considerati spazzatura/rumore di fondo.
    // Dovrai tararlo empiricamente stampando i valori quando il sensore è a riposo.
    float NOISE_THRESHOLD = 200.0f; 

    for (int i = 1; i < FFT_SAMPLES / 2; i++) {
        
        // A. Cerca la componente Dominante (per l'analisi del segnale)
        if (fft_data[i] > power) {
            power = fft_data[i];
            peak_index = i;
        }
        
        // B. Cerca la componente Massima (per il Teorema di Nyquist)
        // Aggiorna l'indice ogni volta che trova una frequenza reale sopra la soglia
        if (fft_data[i] > NOISE_THRESHOLD) {
            highest_index = i;
        }
    }

    // 4. Calcoli finali
    float bin_width_hz = current_sample_rate / (float)FFT_SAMPLES;

    if (max_power) {
        *max_power = power;
    }
    if (highest_freq) {
        // Questa è la f_max da usare per il tuo algoritmo adattivo!
        *highest_freq = (float)highest_index * bin_width_hz;
    }

    // Ritorna la frequenza dominante
    return (float)peak_index * bin_width_hz;
}