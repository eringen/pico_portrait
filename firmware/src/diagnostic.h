#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define DESC_CAP 512
#define HTTP_REQUEST_CAP 1024
#define HTTP_RESPONSE_CAP 4096

typedef struct {
    bool host_enabled, connected, mounted, candidate;
    uint8_t address;
    uint16_t vid, pid;
    uint32_t mounts, disconnects;
    const char *state;
    uint8_t device[18], configuration[DESC_CAP];
    size_t device_len, configuration_len;
} usb_diag_t;

typedef struct {
    usb_diag_t usb;
    const char *wifi_state;
    char ip[16];
    uint8_t oled_address;
    bool oled_ok;
} diagnostic_t;

extern diagnostic_t diagnostic;
uint16_t read_le16(const uint8_t *p);
bool configuration_valid(const uint8_t *data, size_t length);
// Returns 0 on overflow; no truncated JSON is served.
size_t diagnostic_json(char *out, size_t capacity, const diagnostic_t *d);
// Complete HTTP headers required. Route is exact; all non-GET methods rejected.
int http_route(const char *request, size_t length);
size_t http_response(char *out, size_t capacity, int route, const diagnostic_t *d);
