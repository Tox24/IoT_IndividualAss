#pragma once
typedef struct {
    float average;
    float execution_time;
    float sampled_freq;
    int sample_number;
} aggregated_data_t;

typedef struct {
    float execution_time;
    float sampled_freq;
    int sample_number;
    int* samples;
} sample_data_t;

typedef enum {
    DEMO,
    FAST_SAMPLER
} code_personal_mode_t;

code_personal_mode_t MODE = DEMO;