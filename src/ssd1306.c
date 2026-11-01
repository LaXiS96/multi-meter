#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "ssd1306.h"
#include "i2c.h"
#include "font5x7.h"

// TODO font sizes: 8px (text?), 16px (text?), 32px (full height digits only)
//      maybe write a web app that uses canvas to draw a font and then downscales
//      needs to export in ssd1306 format

// Page: 8 pixel wide horizontal row = 1byte (LSB on top); column: 1 pixel wide vertical column
static uint8_t fb[SSD1306_WIDTH * SSD1306_HEIGHT / 8] = {0};

const uint8_t init[] = {
    SSD1306_CONTROL_COMMANDS,
    SSD1306_DISPLAY_OFF,
    SSD1306_MULTIPLEX_RATIO,
    SSD1306_HEIGHT - 1,
    SSD1306_SEG_REMAP_ENABLE,
    SSD1306_COM_SCAN_DECREMENT,
    SSD1306_COM_PINS,
    0x02,
    SSD1306_ADDRESSING_MODE,
    0,
    SSD1306_CONTRAST,
    127,
    SSD1306_ENTIRE_ON_DISABLE,
    SSD1306_INVERT_DISABLE,
    SSD1306_CLOCK_OSC,
    0x80,
    SSD1306_PRECHARGE,
    0xF1,
    // TODO vcomh deselect level?
    SSD1306_CHARGEPUMP,
    0x14,
    SSD1306_DISPLAY_ON,
};

void ssd1306_init(void)
{
    i2c_write(SSD1306_ADDR, init, sizeof(init));
}

void ssd1306_blank(void)
{
    memset(fb, 0, sizeof(fb));
}

void ssd1306_flush(void)
{
    uint8_t data[] = {
        SSD1306_CONTROL_COMMANDS,
        SSD1306_PAGE_ADDRESS,
        0,
        SSD1306_HEIGHT / 8 - 1,
        SSD1306_COLUMN_ADDRESS,
        0,
        SSD1306_WIDTH - 1,
    };
    i2c_write(SSD1306_ADDR, data, sizeof(data));

    i2c_write_begin(SSD1306_ADDR);
    i2c_write_continue((uint8_t[]){SSD1306_CONTROL_DATA}, 1);
    i2c_write_continue(fb, sizeof(fb));
    i2c_write_end();
}

void ssd1306_text(const char *str, uint8_t line, uint8_t col)
{
    uint8_t *f = fb + (line * SSD1306_WIDTH) + col;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        char c = str[i];
        if (c < 32 || c > 127)
            continue;

        uint8_t *g = font5x7 + ((c - 32) * 5);
        for (size_t j = 0; j < 5; j++)
            *(f++) = *(g++);

        f++;
    }
}
