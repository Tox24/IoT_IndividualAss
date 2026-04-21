#pragma once
#include <stdint.h>

void init_fft(int FFT_SAMPLES);
float process_fft(int *samples, int FFT_SAMPLES, float current_sample_rate, float *max_power, float *highest_freq);