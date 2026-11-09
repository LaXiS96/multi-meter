#pragma once

#define ERR_RETURN(v)          \
    {                          \
        if ((err = (v)) != OK) \
            return err;        \
    }

#define ERR_GOTO(v, label)     \
    {                          \
        if ((err = (v)) != OK) \
            goto label;        \
    }

typedef enum
{
    OK,
    I2C_TIMEOUT,
    I2C_NACK,
    I2C_BUS_ERROR,
    I2C_ARBITRATION_LOST,
    I2C_OVERRUN,
} err_t;

static volatile err_t err;
