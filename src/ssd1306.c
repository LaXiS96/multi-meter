#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "ssd1306.h"
#include "i2c.h"
#include "font5x7.h"

// TODO font sizes: 8px (text?), 16px (text?), 32px (full height digits only)

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

    i2c_write_begin(SSD1306_ADDR, (uint8_t[]){SSD1306_CONTROL_DATA});
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

        const uint8_t *g = font5x7 + ((c - 32) * 5);
        for (size_t j = 0; j < 5; j++)
            *(f++) = *(g++);

        f++;
    }
}

void ssd1306_draw_glyph(const ssd1306_font_t *font, int16_t page, int16_t x, uint16_t c)
{
    if (c < font->first_char || c > font->last_char)
        return;

    const ssd1306_glyph_t *glyph = &font->glyphs[c - font->first_char];
    const uint8_t *bitmap = &font->bitmap[glyph->bitmap_offset];

    uint8_t pages = (glyph->height + 7) / 8;
    x += glyph->x_offset;
    for (uint8_t col = 0; col < glyph->width; col++)
    {
        int16_t px = x + col;
        if (px < 0 || px >= SSD1306_WIDTH)
            continue;

        for (uint8_t p = 0; p < pages; p++)
        {
            int16_t dst_page = page + p;
            if (dst_page < 0 || dst_page >= SSD1306_HEIGHT / 8)
                continue;

            fb[dst_page * SSD1306_WIDTH + px] = bitmap[col * pages + p];
        }
    }
}

void ssd1306_print(const ssd1306_font_t *font, int16_t page, int16_t x, const char *text)
{
    int32_t cursor_x = x;
    while (*text)
    {
        uint8_t c = (uint8_t)*text++;
        if (c < font->first_char || c > font->last_char)
            continue;

        // TODO redundant glyph mapping here and in draw_glyph
        const ssd1306_glyph_t *glyph = &font->glyphs[c - font->first_char];
        ssd1306_draw_glyph(font, page, cursor_x, c);
        cursor_x += glyph->x_advance;
    }
}

uint16_t font_get_text_advance(const ssd1306_font_t *font, char *text)
{
    uint16_t width = 0;
    while (*text)
    {
        uint8_t c = (uint8_t)*text++;
        if (c < font->first_char || c > font->last_char)
            continue;
        width += font->glyphs[c - font->first_char].x_advance;
    }
    return width;
}

uint8_t font_get_max_advance(const ssd1306_font_t *font, char first_char, char last_char)
{
    if (first_char < font->first_char)
        first_char = font->first_char;
    if (last_char > font->last_char)
        last_char = font->last_char;
    uint8_t max = 0;
    for (uint16_t c = first_char; c <= last_char; c++)
    {
        uint8_t advance = font->glyphs[c - font->first_char].x_advance;
        if (advance > max)
            max = advance;
    }
    return max;
}
