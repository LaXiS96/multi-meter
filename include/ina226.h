#pragma once

#include <stdint.h>

#include "errors.h"

typedef enum
{
    INA226_REG_CONFIGURATION = 0x00,
    INA226_REG_SHUNT_VOLTAGE = 0x01,
    INA226_REG_BUS_VOLTAGE = 0x02,
    INA226_REG_POWER = 0x03,
    INA226_REG_CURRENT = 0x04,
    INA226_REG_CALIBRATION = 0x05,
    INA226_REG_MASK_ENABLE = 0x06,
    INA226_REG_ALERT_LIMIT = 0x07,
    INA226_REG_MANUFACTURER_ID = 0xFE,
    INA226_REG_DIE_ID = 0xFF,
} ina226_register_t;

typedef enum
{
    INA226_AVERAGE_1,
    INA226_AVERAGE_4,
    INA226_AVERAGE_16,
    INA226_AVERAGE_64,
    INA226_AVERAGE_128,
    INA226_AVERAGE_256,
    INA226_AVERAGE_512,
    INA226_AVERAGE_1024,
} ina226_average_t;

typedef enum
{
    INA226_CONVTIME_140us,
    INA226_CONVTIME_204us,
    INA226_CONVTIME_332us,
    INA226_CONVTIME_588us,
    INA226_CONVTIME_1100us,
    INA226_CONVTIME_2116us,
    INA226_CONVTIME_4156us,
    INA226_CONVTIME_8244us,
} ina226_convtime_t;

typedef enum
{
    INA226_ALERT_LATCHING = (1 << 0),
    INA226_ALERT_POLARITY_INVERTED = (1 << 1),
    INA226_ALERT_ON_CONVERSION_READY = (1 << 10),
    INA226_ALERT_ON_POWER_OVER = (1 << 11),
    INA226_ALERT_ON_BUS_UNDER = (1 << 12),
    INA226_ALERT_ON_BUS_OVER = (1 << 13),
    INA226_ALERT_ON_SHUNT_UNDER = (1 << 14),
    INA226_ALERT_ON_SHUNT_OVER = (1 << 15),
} ina226_alert_t;

typedef enum
{
    INA226_FLAG_OVERFLOW = (1 << 2),
    INA226_FLAG_CONVERSION_READY = (1 << 3),
    INA226_FLAG_ALERT = (1 << 4),
} ina226_flag_t;

/**
 * Note: the ADC resolution is 2.5uV, which corresponds to 25uA with a 100mOhm shunt.
 * Note: built-in averaging does not seem to do oversampling, so max resolution is limited by the ADC resolution.
 */
void ina226_init(uint32_t current_lsb);

void ina226_reset(void);

/**
 * Conversion time = average * (bus_convtime + shunt_convtime)
 */
void ina226_configure(ina226_average_t average, ina226_convtime_t bus_convtime, ina226_convtime_t shunt_convtime);

/**
 * Configures the alert functionality; does not preserve existing configuration.
 *
 * Note: the highest ALERT_ON_* takes precedence as an alert source.
 */
void ina226_alert(ina226_alert_t alert);

err_t ina226_read_bus_voltage(uint32_t *bus_voltage);
err_t ina226_read_current(int32_t *current);
err_t ina226_read_power(uint32_t *power);
err_t ina226_read_flags(ina226_flag_t *flags);
