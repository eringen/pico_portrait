#include "usb_diag.h"
#include "diagnostic.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "host/hcd.h"
#include <stdio.h>
#include <string.h>

static uint8_t transfer[DESC_CAP] __attribute__((aligned(4)));
static uint32_t deadline, generation;
static bool pending;
uint32_t tusb_time_millis_api(void) { return to_ms_since_boot(get_absolute_time()); }
static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }
static void descriptor_done(tuh_xfer_t *xfer);

static void fail(const char *why) {
    pending = false;
    diagnostic.usb.state = why;
    printf("USB %s\n", why);
}
static bool request_descriptor(bool device, uint16_t length) {
    memset(transfer, 0, sizeof(transfer));
    pending = true;
    deadline = now_ms() + 3000;
    bool ok = device ? tuh_descriptor_get_device(diagnostic.usb.address, transfer, length, descriptor_done, generation)
        : tuh_descriptor_get_configuration(diagnostic.usb.address, 0, transfer, length, descriptor_done, generation);
    if (!ok) fail("descriptor_submit_failed");
    return ok;
}
static void print_configuration(const uint8_t *p, size_t n) {
    printf("Configuration[0]: value=%u interfaces=%u attributes=%02x max_power_2mA=%u\n", p[5], p[4], p[7], p[8]);
    for (size_t pos = 0; pos < n; pos += p[pos]) {
        if (p[pos+1] == 4) printf(" Interface %u alt=%u endpoints=%u class=%02x subclass=%02x protocol=%02x\n",
            p[pos+2], p[pos+3], p[pos+4], p[pos+5], p[pos+6], p[pos+7]);
        if (p[pos+1] == 5) printf("  Endpoint %02x attributes=%02x max_packet=%u interval=%u (reported only)\n",
            p[pos+2], p[pos+3], read_le16(p+pos+4), p[pos+6]);
    }
}
static void descriptor_done(tuh_xfer_t *x) {
    usb_diag_t *u = &diagnostic.usb;
    if (!pending || !u->mounted || x->daddr != u->address || x->user_data != generation) return;
    pending = false;
    if (x->result != XFER_RESULT_SUCCESS) { fail("descriptor_transfer_failed"); return; }
    if (!u->device_len) {
        if (x->actual_len != 18 || transfer[0] != 18 || transfer[1] != 1) { fail("bad_device_descriptor"); return; }
        memcpy(u->device, transfer, 18); u->device_len = 18;
        u->vid = read_le16(transfer+8); u->pid = read_le16(transfer+10);
        u->candidate = u->vid == 0x0b4d && u->pid == 0x1123;
        printf("USB VID:PID %04x:%04x candidate=%u bcdUSB=%04x class=%02x ep0=%u configurations=%u\n",
            u->vid, u->pid, u->candidate, read_le16(transfer+2), transfer[4], transfer[7], transfer[17]);
        u->state = "reading_configuration_header";
        request_descriptor(false, 9);
    } else if (!strcmp(u->state, "reading_configuration_header")) {
        if (x->actual_len != 9 || transfer[0] != 9 || transfer[1] != 2) { fail("bad_configuration_header"); return; }
        uint16_t length = read_le16(transfer+2);
        if (length < 9 || length > DESC_CAP) { fail("configuration_too_large_or_invalid"); return; }
        u->state = "reading_configuration";
        request_descriptor(false, length);
    } else {
        if (!configuration_valid(transfer, x->actual_len)) { fail("bad_configuration_descriptor"); return; }
        memcpy(u->configuration, transfer, x->actual_len); u->configuration_len = x->actual_len;
        u->state = "descriptors_ready";
        print_configuration(transfer, x->actual_len);
    }
}
void tuh_mount_cb(uint8_t address) {
    usb_diag_t *u = &diagnostic.usb;
    u->connected = true; u->mounted = true; u->address = address; ++u->mounts;
    u->device_len = u->configuration_len = 0;
    u->candidate = false; u->vid = u->pid = 0;
    ++generation;
    u->state = "reading_device_descriptor";
    printf("USB mounted address=%u (no class drivers, no plotter commands)\n", address);
    request_descriptor(true, 18);
}
void tuh_umount_cb(uint8_t address) {
    (void)address;
    if (!diagnostic.usb.connected && !diagnostic.usb.mounted) return;
    uint32_t mounts = diagnostic.usb.mounts, disconnects = diagnostic.usb.disconnects;
    diagnostic.usb = (usb_diag_t){.host_enabled = true, .state = "disconnected", .mounts = mounts, .disconnects = disconnects+1};
    pending = false; ++generation;
    printf("USB disconnected\n");
}
void usb_diag_init(void) {
#if PORTRAIT_USB_HOST
    diagnostic.usb.host_enabled = tuh_init(0);
    diagnostic.usb.state = diagnostic.usb.host_enabled ? "disconnected" : "host_init_failed";
#else
    diagnostic.usb.state = "disabled_vbus_unverified";
#endif
    printf("USB %s\n", diagnostic.usb.state);
}
void usb_diag_task(void) {
    usb_diag_t *u = &diagnostic.usb;
    if (!u->host_enabled) return;
    tuh_task_ext(0, false);
    bool connected = hcd_port_connect_status(0);
    if (connected && !u->connected) {
        u->connected = true; u->state = "enumerating"; deadline = now_ms()+5000;
        printf("USB attached\n");
    } else if (!connected && u->connected) {
        tuh_umount_cb(u->address);
    }
    if (pending && (int32_t)(now_ms()-deadline) >= 0) {
        // Do not reuse the buffer after timeout. Detach/reboot is required; abort
        // may fail for an active hardware transaction in this TinyUSB version.
        fail("descriptor_timeout_replug_required");
        tuh_edpt_abort_xfer(u->address, 0);
    } else if (u->connected && !u->mounted && !strcmp(u->state,"enumerating") && (int32_t)(now_ms()-deadline) >= 0) {
        fail("enumeration_timeout_replug_required");
    }
}
