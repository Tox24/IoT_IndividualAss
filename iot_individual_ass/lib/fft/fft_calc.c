#include "fft_calc.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "FFT_LIB";

// Puntatori globali allocati una sola volta all'avvio (Risolto il Memory Leak!)
static float *fft_data;
static float *hanning_window;

void init_fft(int FFT_SAMPLES) {
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Errore inizializzazione FFT");
    }
    
    // Allocazione della finestra di Hanning
    hanning_window = (float *)malloc(FFT_SAMPLES * sizeof(float));
    if (!hanning_window) {
        ESP_LOGE(TAG, "Errore nell'allocazione della finestra di Hanning");
        return;
    }
    dsps_wind_hann_f32(hanning_window, FFT_SAMPLES);
    
    // Allocazione del buffer di calcolo per la FFT (Dimensione doppia per i numeri complessi)
    fft_data = (float *)malloc(FFT_SAMPLES * 2 * sizeof(float));
    if (!fft_data) {
        ESP_LOGE(TAG, "Errore nell'allocazione dei dati FFT");
        return;
    }

    ESP_LOGI(TAG, "Modulo FFT pronto (%d campioni)", FFT_SAMPLES);
}


float process_fft(int *samples, int FFT_SAMPLES, float current_sample_rate, float *max_power, float *highest_freq) {
    
    // ==========================================
    // 1. RIMOZIONE DC OFFSET (Centratura dell'onda)
    // ==========================================
    long sum = 0;
    for (int i = 0; i < FFT_SAMPLES; i++) {
        sum += samples[i];
    }
    float mean = (float)sum / (float)FFT_SAMPLES; 

    // ==========================================
    // 2. PREPARAZIONE DEI DATI COMPLESSI
    // ==========================================
    for (int i = 0; i < FFT_SAMPLES; i++) {
        // Sottraggo la media per togliere il "finto zero" (es. 1.65V)
        float centered_sample = (float)samples[i] - mean; 
        
        // Assegno il valore reale (moltiplicato per la finestra di Hanning)
        fft_data[i * 2 + 0] = centered_sample * hanning_window[i];
        // Assegno zero alla parte immaginaria
        fft_data[i * 2 + 1] = 0.0f;
    }
    
    // ==========================================
    // 3. ESECUZIONE FFT
    // ==========================================
    dsps_fft2r_fc32(fft_data, FFT_SAMPLES);
    dsps_bit_rev_fc32(fft_data, FFT_SAMPLES);
    // (dsps_cplx2reC_fc32 rimossa per evitare corruzione dei dati)

    // ==========================================
    // 4. RICERCA DEL PICCO TRAMITE MAGNITUDO
    // ==========================================
    float power = 0;
    int peak_index = 0;
    int highest_index = 0;
    
    // Soglia di rumore alzata per evitare falsi positivi
    float NOISE_THRESHOLD = 50000.0f; 

    // Partiamo da 1 per saltare comunque il bin 0 (ulteriore sicurezza contro residui DC)
    for (int i = 1; i < FFT_SAMPLES / 2; i++) {
        
        // Estraggo Parte Reale e Parte Immaginaria
        float re = fft_data[i * 2 + 0];
        float im = fft_data[i * 2 + 1];
        
        // Teorema di Pitagora per l'energia totale (Magnitudo)
        float magnitude = sqrt((re * re) + (im * im));
        
        // A. Cerca la componente Dominante (per l'analisi del segnale)
        if (magnitude > power) {
            power = magnitude;
            peak_index = i;
        }
        
        // B. Cerca la componente Massima reale (per il Teorema di Nyquist)
        if (magnitude > NOISE_THRESHOLD) {
            highest_index = i;
        }
    }

    // ==========================================
    // 5. CALCOLI FINALI
    // ==========================================
    float bin_width_hz = current_sample_rate / (float)FFT_SAMPLES;

    if (max_power) {
        *max_power = power;
    }
    if (highest_freq) {
        // Questa è la frequenza massima per il tuo algoritmo adattivo
        *highest_freq = (float)highest_index * bin_width_hz;
    }

    // Ritorna la frequenza dominante individuata
    return (float)peak_index * bin_width_hz;
}