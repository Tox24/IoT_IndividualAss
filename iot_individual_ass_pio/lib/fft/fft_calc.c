#include "fft_calc.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

static const char *TAG = "FFT_LIB";

static float *fft_data = NULL;
static float *hanning_window = NULL;
static int fft_size = 0;
static bool fft_ready = false;

static bool is_power_of_two(int value) {
    return (value > 0) && ((value & (value - 1)) == 0);
}

static void release_fft_buffers(void) {
    if (fft_data) {
        free(fft_data);
        fft_data = NULL;
    }
    if (hanning_window) {
        free(hanning_window);
        hanning_window = NULL;
    }
    fft_size = 0;
    fft_ready = false;
}

void init_fft(int FFT_SAMPLES) {
    if (!is_power_of_two(FFT_SAMPLES)) {
        ESP_LOGE(TAG, "FFT_SAMPLES deve essere una potenza di 2 (valore: %d)", FFT_SAMPLES);
        return;
    }

    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Errore inizializzazione FFT");
        return;
    }

    if (fft_size != FFT_SAMPLES) {
        release_fft_buffers();
    }

    if (hanning_window == NULL) {
        hanning_window = (float *)malloc((size_t)FFT_SAMPLES * sizeof(float));
    }
    if (!hanning_window) {
        ESP_LOGE(TAG, "Errore nell'allocazione della finestra di Hanning");
        release_fft_buffers();
        return;
    }
    dsps_wind_hann_f32(hanning_window, FFT_SAMPLES);

    if (fft_data == NULL) {
        fft_data = (float *)malloc((size_t)FFT_SAMPLES * 2U * sizeof(float));
    }
    if (!fft_data) {
        ESP_LOGE(TAG, "Errore nell'allocazione dei dati FFT");
        release_fft_buffers();
        return;
    }

    fft_size = FFT_SAMPLES;
    fft_ready = true;

    ESP_LOGI(TAG, "Modulo FFT pronto (%d campioni)", FFT_SAMPLES);
}


float process_fft(int *samples, int FFT_SAMPLES, float current_sample_rate, float *max_power, float *highest_freq) {
    if (max_power) {
        *max_power = 0.0f;
    }
    if (highest_freq) {
        *highest_freq = 0.0f;
    }

    if (!samples || current_sample_rate <= 0.0f || FFT_SAMPLES <= 0 || !is_power_of_two(FFT_SAMPLES)) {
        ESP_LOGE(TAG, "Parametri FFT non validi");
        return 0.0f;
    }

    if (!fft_ready || fft_size != FFT_SAMPLES || !fft_data || !hanning_window) {
        ESP_LOGE(TAG, "FFT non inizializzata o dimensione incoerente. Richiesto init_fft(%d)", FFT_SAMPLES);
        return 0.0f;
    }

    int64_t sum = 0;
    for (int i = 0; i < FFT_SAMPLES; ++i) {
        sum += samples[i];
    }
    const float mean = (float)sum / (float)FFT_SAMPLES;

    for (int i = 0; i < FFT_SAMPLES; ++i) {
        const float centered_sample = (float)samples[i] - mean;
        fft_data[i * 2 + 0] = centered_sample * hanning_window[i];
        fft_data[i * 2 + 1] = 0.0f;
    }

    dsps_fft2r_fc32(fft_data, FFT_SAMPLES);
    dsps_bit_rev_fc32(fft_data, FFT_SAMPLES);

    float peak_mag_sq = 0.0f;
    float mean_mag_sq = 0.0f;
    int peak_index = 0;
    const int half_spectrum = FFT_SAMPLES / 2;
    
    for (int i = 1; i < half_spectrum; ++i) {
        const float re = fft_data[i * 2 + 0];
        const float im = fft_data[i * 2 + 1];
        const float mag_sq = (re * re) + (im * im);
        mean_mag_sq += mag_sq;

        if (mag_sq > peak_mag_sq) {
            peak_mag_sq = mag_sq;
            peak_index = i;
        }
    }

    if (half_spectrum > 1) {
        mean_mag_sq /= (float)(half_spectrum - 1);
    }

    const float relative_threshold = peak_mag_sq * 0.1f;
    const float noise_threshold = mean_mag_sq * 8.0f;
    const float detection_threshold = fmaxf(relative_threshold, noise_threshold);

    int highest_index = 0;

    for (int i = 1; i < half_spectrum; ++i) {
        const float re = fft_data[i * 2 + 0];
        const float im = fft_data[i * 2 + 1];
        const float mag_sq = (re * re) + (im * im);

        if (mag_sq > detection_threshold) {
            highest_index = i;
        }
    }

    const float bin_width_hz = current_sample_rate / (float)FFT_SAMPLES;
    const float peak_magnitude = sqrtf(peak_mag_sq);

    if (max_power) {
        *max_power = peak_magnitude;
    }
    if (highest_freq) {
        *highest_freq = (float)highest_index * bin_width_hz;
    }

    return (float)peak_index * bin_width_hz;
}