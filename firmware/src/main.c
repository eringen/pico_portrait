#include "config.h"
#include "diagnostic.h"
#include "http.h"
#include "oled.h"
#include "usb_diag.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    stdio_init_all(); // UART1: GP4 TX / GP5 RX, 115200 8N1. No USB stdio.
    printf("\nPortrait 0.2.0 NO-MOTION diagnostic. Host build=%u\n", PORTRAIT_USB_HOST);
    diagnostic.oled_address = oled_init();
    diagnostic.oled_ok = oled_lines("PORTRAIT PICO", "WIFI STARTING", "NO MOTION", "SSD1306 ASSUMED");
    bool wifi_init = cyw43_arch_init() == 0;
    bool joining = false, http_ready = false;
    uint32_t join_started = to_ms_since_boot(get_absolute_time());
    if (!wifi_init) diagnostic.wifi_state = "init_failed";
    else if (!PORTRAIT_WIFI_SSID[0]) diagnostic.wifi_state = "disabled_no_credentials";
    else {
        cyw43_arch_enable_sta_mode();
        int result = cyw43_arch_wifi_connect_async(PORTRAIT_WIFI_SSID, PORTRAIT_WIFI_PASSWORD,
            PORTRAIT_WIFI_PASSWORD[0] ? CYW43_AUTH_WPA2_AES_PSK : CYW43_AUTH_OPEN);
        joining = result == 0;
        diagnostic.wifi_state = joining ? "joining" : "join_start_failed";
    }
    usb_diag_init();
    uint32_t last_display = 0;
    while (true) {
        usb_diag_task();
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (wifi_init) {
            cyw43_arch_poll();
            int link = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
            if (link == CYW43_LINK_UP) {
                joining = false;
                diagnostic.wifi_state = "up";
                const char *ip = ip4addr_ntoa(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]));
                if (strcmp(ip, diagnostic.ip)) {
                    snprintf(diagnostic.ip, sizeof(diagnostic.ip), "%s", ip);
                    printf("WiFi up: http://%s/\n", diagnostic.ip);
                }
                if (!http_ready) http_ready = http_start();
                if (!http_ready) diagnostic.wifi_state = "http_start_failed";
            } else {
                if (!strcmp(diagnostic.wifi_state, "up") || !strcmp(diagnostic.wifi_state, "http_start_failed")) {
                    diagnostic.wifi_state = "lost_restart_required";
                    strcpy(diagnostic.ip, "0.0.0.0");
                }
                if (joining && (link < 0 || now-join_started >= 30000)) {
                    joining = false;
                    diagnostic.wifi_state = "join_failed_restart_required";
                    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
                    printf("WiFi join failed; link=%d. Restart to retry.\n", link);
                }
            }
        }
        if (now-last_display >= 1000) {
            last_display = now;
            char usb_line[24];
            if (diagnostic.usb.device_len) snprintf(usb_line,sizeof(usb_line),"USB %04X:%04X",diagnostic.usb.vid,diagnostic.usb.pid);
            else snprintf(usb_line,sizeof(usb_line),"USB %s",diagnostic.usb.host_enabled ? (diagnostic.usb.connected?"ENUMERATING":"DISCONNECTED") : "DISABLED");
            diagnostic.oled_ok = oled_lines("PORTRAIT PICO", !strcmp(diagnostic.wifi_state,"up") ? diagnostic.ip : diagnostic.wifi_state,
                usb_line, "JOBS OFF - NO MOTION");
        }
        sleep_ms(1);
    }
}
