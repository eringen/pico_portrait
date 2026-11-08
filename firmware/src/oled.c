#include "oled.h"
#include "config.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
_Static_assert(PORTRAIT_OLED_HEIGHT == 32 || PORTRAIT_OLED_HEIGHT == 64, "OLED height must be 32 or 64");
_Static_assert(PORTRAIT_OLED_SDA % 4 == 0 && PORTRAIT_OLED_SCL == PORTRAIT_OLED_SDA + 1, "Use an I2C0 SDA/SCL pair");
static uint8_t address;
static uint8_t pixels[1 + 128*PORTRAIT_OLED_HEIGHT/8];
// Compact original 3x5 uppercase font; each byte is one row (bits 2..0).
static const uint8_t glyphs[][5] = {
 {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},
 {3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},
 {5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,3,1},{6,5,6,5,5},
 {3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},
 {5,5,2,2,2},{7,1,2,4,7},
 {7,5,5,5,7},{2,6,2,2,7},{6,1,2,4,7},{6,1,2,1,6},{5,5,7,1,1},
 {7,4,6,1,6},{3,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,6},
 {0,0,0,0,2},{0,2,0,2,0},{0,0,7,0,0},{6,1,2,0,2}
};
static bool cmd(uint8_t value) {
    uint8_t bytes[] = {0x80, value};
    return i2c_write_timeout_us(i2c0, address, bytes, 2, false, 2000) == 2;
}
uint8_t oled_init(void) {
    i2c_init(i2c0, 100000);
    gpio_set_function(PORTRAIT_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PORTRAIT_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PORTRAIT_OLED_SDA); gpio_pull_up(PORTRAIT_OLED_SCL);
    // Probe only expected OLED addresses, with a read (no controller writes).
    for (uint8_t a = 0x3c; a <= 0x3d; ++a) {
        uint8_t byte;
        if (i2c_read_timeout_us(i2c0, a, &byte, 1, false, 2000) == 1) {
            printf("I2C ACK 0x%02x (SSD1306 unverified)\n", a);
            if (!address) address = a;
        }
    }
    if (!address) return 0;
    const uint8_t init[] = {0xae,0x20,0,0x40,0xa1,0xa8,PORTRAIT_OLED_HEIGHT-1,
        0xc8,0xd3,0,0xda,PORTRAIT_OLED_HEIGHT==32?0x02:0x12,0xd5,0x80,0xd9,0xf1,
        0xdb,0x30,0x81,0x60,0xa4,0xa6,0x8d,0x14,0xaf};
    for (size_t i=0;i<sizeof(init);++i) if (!cmd(init[i])) { address=0; break; }
    return address;
}
bool oled_lines(const char *a, const char *b, const char *c, const char *d) {
    if (!address) return false;
    memset(pixels, 0, sizeof(pixels)); pixels[0]=0x40;
    const char *lines[]={a,b,c,d};
    for (unsigned row=0;row<4;++row) for (unsigned x=0;lines[row][x] && x<21;++x) {
        int ch=toupper((unsigned char)lines[row][x]);
        int index = ch>='A' && ch<='Z' ? ch-'A' : ch>='0' && ch<='9' ? 26+ch-'0'
            : ch=='.'?36:ch==':'?37:ch=='-'?38:ch=='?'?39:-1;
        if (index<0) continue;
        for (unsigned y=0;y<5;++y) for (unsigned col=0;col<3;++col)
            if (glyphs[index][y] & (4>>col)) {
                pixels[1+row*128+x*6+col*2] |= (uint8_t)(1<<(y+1));
                pixels[1+row*128+x*6+col*2+1] |= (uint8_t)(1<<(y+1));
            }
    }
    const uint8_t window[]={0x21,0,127,0x22,0,PORTRAIT_OLED_HEIGHT/8-1};
    for (size_t i=0;i<sizeof(window);++i) if (!cmd(window[i])) return false;
    // At 100kHz a 128x32 frame takes about 47ms. Refresh only once per second.
    return i2c_write_timeout_us(i2c0,address,pixels,sizeof(pixels),false,110000)==(int)sizeof(pixels);
}
