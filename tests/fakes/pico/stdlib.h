#pragma once
#include <stdint.h>
extern uint32_t fake_time;
static inline uint32_t get_absolute_time(void) { return fake_time; }
static inline uint32_t to_ms_since_boot(uint32_t t) { return t; }
