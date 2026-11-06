#pragma once

#define SSD1306_CONTROL_COMMANDS 0x00
#define SSD1306_CONTROL_DATA 0x40

#define SSD1306_ADDRESSING_MODE 0x20
#define SSD1306_COLUMN_ADDRESS 0x21
#define SSD1306_PAGE_ADDRESS 0x22
#define SSD1306_CONTRAST 0x81
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_SEG_REMAP_DISABLE 0xA0
#define SSD1306_SEG_REMAP_ENABLE 0xA1
#define SSD1306_ENTIRE_ON_DISABLE 0xA4
#define SSD1306_ENTIRE_ON_ENABLE 0xA5
#define SSD1306_INVERT_DISABLE 0xA6
#define SSD1306_INVERT_ENABLE 0xA7
#define SSD1306_MULTIPLEX_RATIO 0xA8
#define SSD1306_DISPLAY_OFF 0xAE
#define SSD1306_DISPLAY_ON 0xAF
#define SSD1306_COM_SCAN_INCREMENT 0xC0
#define SSD1306_COM_SCAN_DECREMENT 0xC8
#define SSD1306_OFFSET 0xD3
#define SSD1306_CLOCK_OSC 0xD5
#define SSD1306_COM_PINS 0xDA
#define SSD1306_PRECHARGE 0xD9
#define SSD1306_NOP 0xE3

typedef struct
{
    uint16_t bitmap_offset;
    uint8_t width;
    uint8_t height;
    int8_t x_offset;
    uint8_t x_advance;
} ssd1306_glyph_t;

typedef struct
{
    uint8_t font_height;
    uint16_t first_char;
    uint16_t last_char;
    const ssd1306_glyph_t *glyphs;
    const uint8_t *bitmap;
} ssd1306_font_t;

void ssd1306_init(void);
void ssd1306_blank(void);
void ssd1306_flush(void);
void ssd1306_text(const char *str, uint8_t line, uint8_t col);
void ssd1306_draw_glyph(const ssd1306_font_t *font, int16_t page, int16_t x, uint16_t c);
void ssd1306_print(const ssd1306_font_t *font, int16_t page, int16_t x, const char *text);

/** Calculates the total cursor advance of the given `text` */
uint16_t font_get_text_advance(const ssd1306_font_t *font, char *text);

/** Calculates maximum cursor advance across font glyphs from `first_char` to `last_char` */
uint8_t font_get_max_advance(const ssd1306_font_t *font, char first_char, char last_char);
