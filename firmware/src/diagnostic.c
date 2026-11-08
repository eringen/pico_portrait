#include "diagnostic.h"
#include <stdio.h>
#include <string.h>

diagnostic_t diagnostic = {.usb = {.state = "disabled"}, .wifi_state = "disabled", .ip = "0.0.0.0"};
uint16_t read_le16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }

bool configuration_valid(const uint8_t *p, size_t n) {
    if (n < 9 || n > DESC_CAP || p[0] != 9 || p[1] != 2 || read_le16(p + 2) != n) return false;
    bool have_interface = false;
    for (size_t pos = 0; pos < n;) {
        if (n - pos < 2 || p[pos] < 2 || p[pos] > n - pos) return false;
        uint8_t type = p[pos + 1], len = p[pos];
        if (type == 4) {
            if (len < 9) return false;
            have_interface = true;
        }
        if (type == 5 && (!have_interface || len < 7)) return false;
        pos += len;
    }
    return true;
}

static void hex_bytes(char *out, const uint8_t *in, size_t count) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < count; ++i) {
        out[2*i] = hex[in[i] >> 4]; out[2*i + 1] = hex[in[i] & 15];
    }
    out[2*count] = 0;
}

size_t diagnostic_json(char *out, size_t cap, const diagnostic_t *d) {
    if (d->usb.device_len > 18 || d->usb.configuration_len > DESC_CAP) return 0;
    char dev[37], config[DESC_CAP * 2 + 1];
    hex_bytes(dev, d->usb.device, d->usb.device_len);
    hex_bytes(config, d->usb.configuration, d->usb.configuration_len);
    // All string fields are internal enum literals or an IPv4 address. Device-provided
    // strings are intentionally not requested; raw USB bytes are hex encoded.
    int n = snprintf(out, cap,
        "{\"api_version\":1,\"firmware\":\"0.2.0\",\"stage\":2,"
        "\"wifi\":{\"state\":\"%s\",\"ip\":\"%s\"},"
        "\"oled\":{\"address\":%u,\"io_ok\":%s,\"controller_verified\":false},"
        "\"usb\":{\"host_enabled\":%s,\"state\":\"%s\",\"connected\":%s,\"mounted\":%s,"
        "\"address\":%u,\"vid\":%u,\"pid\":%u,\"portrait_candidate\":%s,"
        "\"mounts\":%lu,\"disconnects\":%lu,\"device_descriptor_hex\":\"%s\","
        "\"configuration_index\":0,\"configuration_descriptor_hex\":\"%s\"},"
        "\"capabilities\":{\"status_query\":false,\"job_submission\":false,\"svg\":false},"
        "\"motion_enabled\":false,\"job\":{\"state\":\"unsupported\","
        "\"command_bytes_sent\":0,\"plotting_complete\":null}}",
        d->wifi_state, d->ip, d->oled_address, d->oled_ok ? "true" : "false",
        d->usb.host_enabled ? "true" : "false", d->usb.state,
        d->usb.connected ? "true" : "false", d->usb.mounted ? "true" : "false",
        d->usb.address, d->usb.vid, d->usb.pid, d->usb.candidate ? "true" : "false",
        (unsigned long)d->usb.mounts, (unsigned long)d->usb.disconnects, dev, config);
    return n > 0 && (size_t)n < cap ? (size_t)n : 0;
}

int http_route(const char *r, size_t len) {
    if (len < 4 || len > HTTP_REQUEST_CAP || memchr(r, 0, len)) return 400;
    const char *end = NULL;
    for (size_t i = 0; i + 3 < len; ++i)
        if (!memcmp(r+i, "\r\n\r\n", 4)) { end = r+i+4; break; }
    if (!end) return 400;
    const char *line = NULL;
    for (const char *p = r; p + 1 < end; ++p)
        if (p[0] == '\r' && p[1] == '\n') { line = p; break; }
    if (!line) return 400;
    const char *space = memchr(r, ' ', (size_t)(line-r));
    if (!space) return 400;
    if (space-r != 3 || memcmp(r, "GET", 3)) return 405;
    const char *path = space+1;
    space = memchr(path, ' ', (size_t)(line-path));
    if (!space || line-space != 9 ||
        (memcmp(space+1, "HTTP/1.0", 8) && memcmp(space+1, "HTTP/1.1", 8))) return 400;
    size_t n = (size_t)(space-path);
    if (n == 1 && path[0] == '/') return 200;
    if (n == 11 && !memcmp(path, "/api/status", 11)) return 201; // internal route code, HTTP 200
    return 404;
}

size_t http_response(char *out, size_t cap, int route, const diagnostic_t *d) {
    // Called only from the single-core polling loop. Static scratch avoids
    // placing the HTTP body on the RP2040 scratch-bank stack. Not reentrant.
    static char body[3072];
    const char *type = "text/plain; charset=utf-8", *status = "200 OK";
    if (route == 201) {
        if (!diagnostic_json(body, sizeof(body), d)) return 0;
        type = "application/json";
    } else if (route == 200) {
        type = "text/html; charset=utf-8";
        snprintf(body, sizeof(body), "<!doctype html><meta name=viewport content='width=device-width'>"
            "<title>Portrait diagnostic</title><h1>Portrait Pico W</h1>"
            "<p>Stage 2: no-motion USB descriptor diagnostic.</p>"
            "<p><a href='/api/status'>Read current status and descriptors</a></p>"
            "<p>Plotter queries, SVG and jobs are unavailable. USB completion is not plotting completion.</p>");
    } else {
        switch (route) {
            case 405: status = "405 Method Not Allowed"; break;
            case 404: status = "404 Not Found"; break;
            case 431: status = "431 Request Header Fields Too Large"; break;
            default: status = "400 Bad Request"; break;
        }
        snprintf(body, sizeof(body), "%s\n", status);
    }
    int n = snprintf(out, cap, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
        "Connection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n%s\r\n%s",
        status, type, (unsigned)strlen(body), route == 405 ? "Allow: GET\r\n" : "", body);
    return n > 0 && (size_t)n < cap ? (size_t)n : 0;
}
