#pragma once

void i2c_init(void);

/**
 * @param address 7bit address, LSB-aligned
 */
void i2c_write(uint8_t address, const uint8_t *data, const size_t len);

void i2c_write_begin(uint8_t address);
void i2c_write_continue(const uint8_t *data, const size_t len);
void i2c_write_end(void);

void i2c_read(uint8_t address, uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len);
