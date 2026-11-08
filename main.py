"""Portrait Pico W, stage 1: OLED + Wi-Fi + read-only status page.
No USB host, no motor commands. Run with Pico connected to computer only.
"""
from machine import Pin, I2C
import framebuf
import network
import socket
import time
import json
import config

class OLED:
    """Minimal SSD1306 I2C display test; controller remains to be verified."""
    def __init__(self, bus, address, height):
        if height not in (32, 64):
            raise ValueError("OLED_HEIGHT must be 32 or 64")
        self.bus, self.address, self.height = bus, address, height
        self.data = bytearray(128 * height // 8)
        self.fb = framebuf.FrameBuffer(self.data, 128, height, framebuf.MONO_VLSB)
        for value in (0xAE, 0x20, 0, 0x40, 0xA1, 0xA8, height-1,
                      0xC8, 0xD3, 0, 0xDA, 0x02 if height == 32 else 0x12,
                      0xD5, 0x80, 0xD9, 0xF1, 0xDB, 0x30, 0x81, 0x60,
                      0xA4, 0xA6, 0x8D, 0x14, 0xAF):
            self.cmd(value)
    def cmd(self, value):
        self.bus.writeto(self.address, bytes((0x80, value)))
    def border_test(self):
        self.fb.fill(0)
        self.fb.rect(0, 0, 128, self.height, 1)
        self.fb.text("128 x " + str(self.height), 24, 12, 1)
        self.show()
    def lines(self, *lines):
        self.fb.fill(0)
        for n, line in enumerate(lines[:self.height//8]):
            self.fb.text(str(line)[:16], 0, n*8, 1)
        self.show()
    def show(self):
        for value in (0x21, 0, 127, 0x22, 0, self.height//8-1):
            self.cmd(value)
        self.bus.writeto(self.address, b'\x40' + self.data)

def main():
    bus = I2C(0, sda=Pin(config.SDA_PIN), scl=Pin(config.SCL_PIN), freq=100000)
    found = bus.scan()
    print("I2C addresses:", [hex(a) for a in found])
    address = next((a for a in (0x3C, 0x3D) if a in found), None)
    if address is None:
        raise RuntimeError("No OLED at 0x3C/0x3D. Check 3.3V, GND, SDA, SCL.")
    oled = OLED(bus, address, config.OLED_HEIGHT)
    oled.border_test()
    print("Check the rectangle reaches all four display edges.")
    time.sleep(3)
    oled.lines("PORTRAIT PICO", "OLED TEST ROW 2", "OLED TEST ROW 3", "SSD1306 assumed")
    print("Check all four text rows are readable. I2C ACK alone does not prove controller type.")
    time.sleep(3)
    if not config.WIFI_SSID:
        oled.lines("PORTRAIT PICO", "OLED OK?", "Set WiFi in", "config.py")
        print("Set WIFI_SSID and WIFI_PASSWORD in config.py, then restart.")
        return
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(config.WIFI_SSID, config.WIFI_PASSWORD)
    started = time.ticks_ms()
    while not wlan.isconnected() and time.ticks_diff(time.ticks_ms(), started) < 25000:
        oled.lines("PORTRAIT PICO", "WiFi joining...", "USB host: later", "No motion")
        time.sleep_ms(500)
    if not wlan.isconnected():
        status = wlan.status()
        wlan.disconnect()
        oled.lines("WiFi failed", "Check credentials", "Use 2.4GHz WiFi", "Restart to retry")
        raise RuntimeError("WiFi join timed out; status=" + str(status))
    ip = wlan.ifconfig()[0]
    oled.lines("PORTRAIT PICO", ip, "Web status ready", "USB host: later")
    print("Open http://" + ip + "/ on the same LAN")
    server = socket.socket()
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("0.0.0.0", 80))
    server.listen(1)
    server.settimeout(1)
    try:
        while True:
            if not wlan.isconnected():
                oled.lines("WiFi lost", "Restart to retry", "No plotter link")
                return
            try:
                client, _ = server.accept()
            except OSError:
                continue
            try:
                client.settimeout(2)
                request = b""
                while b"\r\n\r\n" not in request and len(request) < 2048:
                    chunk = client.recv(min(256, 2048-len(request)))
                    if not chunk:
                        break
                    request += chunk
                first = request.split(b"\r\n", 1)[0].split()
                status = "200 OK"
                content_type = "text/html; charset=utf-8"
                if len(first) < 2 or first[0] != b"GET":
                    status, body = "405 Method Not Allowed", "GET only"
                elif first[1] == b"/api/status":
                    content_type = "application/json"
                    body = json.dumps({"stage": 1, "wifi": True, "ip": ip,
                        "oled_address": hex(address), "usb_host": "not implemented",
                        "plotter": "not connected", "motion_enabled": False})
                elif first[1] == b"/":
                    body = ('<!doctype html><meta name="viewport" content="width=device-width">'
                        '<title>Portrait Pico</title><style>body{font:18px system-ui;'
                        'max-width:600px;margin:40px auto;padding:20px;background:#14212b;color:#eee}'
                        'a{color:#7de0ce}</style><h1>Portrait Pico W</h1>'
                        '<p>Stage 1: Wi-Fi and OLED diagnostic.</p><p>Network connection works.</p>'
                        '<p>USB host and plotting are not implemented in this test.</p>'
                        '<p><a href="/api/status">View JSON status</a></p>')
                else:
                    status, body = "404 Not Found", "Not found"
                payload = body.encode()
                header = ("HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
                    "Connection: close\r\nCache-Control: no-store\r\n\r\n" %
                    (status, content_type, len(payload))).encode()
                pending = memoryview(header + payload)
                while len(pending):
                    sent = client.send(pending)
                    if not sent:
                        break
                    pending = pending[sent:]
            except OSError as error:
                print("HTTP client closed or timed out:", error)
            finally:
                client.close()
    finally:
        server.close()

if __name__ == "__main__":
    main()
