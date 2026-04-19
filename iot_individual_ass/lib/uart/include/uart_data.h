#pragma once
#include <stdint.h>

#define BUFFER_SIZE 128

void init_uart(void);
int read_clean_from_uart(int16_t *values);