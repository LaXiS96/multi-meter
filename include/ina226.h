#pragma once

#include <stdint.h>

#define INA226_CONFIGURATION 0x00
#define INA226_SHUNT_VOLTAGE 0x01
#define INA226_BUS_VOLTAGE 0x02
#define INA226_POWER 0x03
#define INA226_CURRENT 0x04
#define INA226_CALIBRATION 0x05
#define INA226_MASK_ENABLE 0x06
#define INA226_ALERT_LIMIT 0x07
#define INA226_MANUFACTURER_ID 0xFE
#define INA226_DIE_ID 0xFF

typedef enum
{
    INA226_AVERAGE_1 = 0b000,
    INA226_AVERAGE_4 = 0b001,
    INA226_AVERAGE_16 = 0b010,
    INA226_AVERAGE_64 = 0b011,
    INA226_AVERAGE_128 = 0b100,
    INA226_AVERAGE_256 = 0b101,
    INA226_AVERAGE_512 = 0b110,
    INA226_AVERAGE_1024 = 0b111,
} ina226_average_t;

typedef enum
{
    INA226_CONVTIME_140us = 0b000,
    INA226_CONVTIME_204us = 0b001,
    INA226_CONVTIME_332us = 0b010,
    INA226_CONVTIME_588us = 0b011,
    INA226_CONVTIME_1100us = 0b100,
    INA226_CONVTIME_2116us = 0b101,
    INA226_CONVTIME_4156us = 0b110,
    INA226_CONVTIME_8244us = 0b111,
} ina226_convtime_t;

void ina226_init(void);
void ina226_reset(void);
void ina226_configure(ina226_average_t average, ina226_convtime_t bus_convtime, ina226_convtime_t shunt_convtime);
uint32_t ina226_get_voltage(void);
int32_t ina226_get_current(void);
uint32_t ina226_get_power(void);
