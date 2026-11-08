#pragma once
#include <stdbool.h>
#include <stdint.h>
#define XFER_RESULT_SUCCESS 0
struct tuh_xfer_s;
typedef struct tuh_xfer_s tuh_xfer_t;
typedef void (*tuh_xfer_cb_t)(tuh_xfer_t *);
struct tuh_xfer_s { uint8_t daddr; int result; uint32_t actual_len; uintptr_t user_data; };
bool tuh_descriptor_get_device(uint8_t, void *, uint16_t, tuh_xfer_cb_t, uintptr_t);
bool tuh_descriptor_get_configuration(uint8_t, uint8_t, void *, uint16_t, tuh_xfer_cb_t, uintptr_t);
bool tuh_init(uint8_t);
void tuh_task_ext(uint32_t, bool);
bool tuh_edpt_abort_xfer(uint8_t, uint8_t);
