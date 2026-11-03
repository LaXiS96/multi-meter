#include "ina226.h"

#include <stdint.h>
#include <ch32v00x.h>

#include "bits.h"
#include "i2c.h"

static uint32_t _current_lsb = 25; // uA

void ina226_init(uint32_t current_lsb)
{
    ina226_reset();

    // Note: no point in going lower than 25uA LSB since that's the ADC max resolution
    // CAL = 0.00512 / (uA * 0.1Ohm)
    // Current_LSB = 25uA = 819.2mA / 2^15, CAL = 2048
    // Current_LSB = 50uA = 1.6384A / 2^15, CAL = 1024
    // Current_LSB = 100uA = 3.2768A / 2^15, CAL = 512
    // Current_LSB = 200uA = 6.5526A / 2^15, CAL = 256
    _current_lsb = current_lsb;
    uint32_t cal = (5120 * 1000000ULL) / ((uint64_t)current_lsb * INA226_SHUNT_UOHM);
    i2c_write(INA226_ADDR, (uint8_t[]){INA226_REG_CALIBRATION, u16be(cal)}, 3);
}

void ina226_reset(void)
{
    i2c_write(INA226_ADDR, (uint8_t[]){INA226_REG_CONFIGURATION, u16be(0x8000)}, 3);
}

// TODO allow configuring modes
void ina226_configure(ina226_average_t average, ina226_convtime_t bus_convtime, ina226_convtime_t shunt_convtime)
{
    uint8_t buf[2] = {0};
    i2c_read(INA226_ADDR, (uint8_t[]){INA226_REG_CONFIGURATION}, 1, buf, 2);
    uint16_t config = u8to16(buf[0], buf[1]);
    bsm(config, 9, 0b111, average);
    bsm(config, 6, 0b111, bus_convtime);
    bsm(config, 3, 0b111, shunt_convtime);
    i2c_write(INA226_ADDR, (uint8_t[]){INA226_REG_CONFIGURATION, u16be(config)}, 3);
}

void ina226_alert(ina226_alert_t alert)
{
    i2c_write(INA226_ADDR, (uint8_t[]){INA226_REG_MASK_ENABLE, u16be(alert)}, 3);
}

uint32_t ina226_read_bus_voltage(void)
{
    uint8_t buf[2] = {0};
    i2c_read(INA226_ADDR, (uint8_t[]){INA226_REG_BUS_VOLTAGE}, 1, buf, 2);
    return u8to16(buf[0], buf[1]) * 125 / 100; // 1.25mV LSB
}

int32_t ina226_read_current(void)
{
    uint8_t buf[2] = {0};
    i2c_read(INA226_ADDR, (uint8_t[]){INA226_REG_CURRENT}, 1, buf, 2);
    return (int16_t)(u8to16(buf[0], buf[1])) * (int32_t)_current_lsb;
}

uint32_t ina226_read_power(void)
{
    uint8_t buf[2] = {0};
    i2c_read(INA226_ADDR, (uint8_t[]){INA226_REG_POWER}, 1, buf, 2);
    return u8to16(buf[0], buf[1]) * _current_lsb * 25; // Power_LSB = Current_LSB * 25
}

ina226_flags_t ina226_read_flags(void)
{
    uint8_t buf[2] = {0};
    i2c_read(INA226_ADDR, (uint8_t[]){INA226_REG_MASK_ENABLE}, 1, buf, 2);
    return (uint16_t)u8to16(buf[0], buf[1]);
}
