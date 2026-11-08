#pragma once
#include <stdbool.h>
#include <stdint.h>
uint8_t oled_init(void);
bool oled_lines(const char *a, const char *b, const char *c, const char *d);
