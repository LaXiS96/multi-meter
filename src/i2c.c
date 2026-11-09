#include <ch32v00x.h>

#include "i2c.h"
#include "time.h"

static err_t i2c_check_errors(void)
{
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        I2C_ClearFlag(I2C1, I2C_FLAG_AF);
        return I2C_NACK;
    }
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR))
    {
        I2C_GenerateSTOP(I2C1, ENABLE);
        I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
        return I2C_BUS_ERROR;
    }
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ARLO))
    {
        I2C_ClearFlag(I2C1, I2C_FLAG_ARLO);
        return I2C_ARBITRATION_LOST;
    }
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_OVR))
    {
        // Note: should never happen in master mode
        return I2C_OVERRUN;
    }
    return OK;
}

static err_t i2c_wait_event(uint32_t event)
{
    uint32_t start = time_ms;
    while (!I2C_CheckEvent(I2C1, event))
    {
        ERR_RETURN(i2c_check_errors());
        if (time_ms - start >= I2C_TIMEOUT_MS)
            // TODO do we need to do anything to ensure the next transaction can start?
            return I2C_TIMEOUT;
    }
    return OK;
}

static err_t i2c_wait_flag(uint32_t flag, FlagStatus expected)
{
    uint32_t start = time_ms;
    while (I2C_GetFlagStatus(I2C1, flag) != expected)
    {
        ERR_RETURN(i2c_check_errors());
        if (time_ms - start >= I2C_TIMEOUT_MS)
            return I2C_TIMEOUT;
    }
    return OK;
}

void i2c_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_Init(GPIOC, &(GPIO_InitTypeDef){
                         .GPIO_Pin = GPIO_Pin_1,
                         .GPIO_Mode = GPIO_Mode_AF_OD,
                         .GPIO_Speed = GPIO_Speed_30MHz,
                     });
    GPIO_Init(GPIOC, &(GPIO_InitTypeDef){
                         .GPIO_Pin = GPIO_Pin_2,
                         .GPIO_Mode = GPIO_Mode_AF_OD,
                         .GPIO_Speed = GPIO_Speed_30MHz,
                     });

    I2C_Init(I2C1, &(I2C_InitTypeDef){
                       .I2C_Ack = I2C_Ack_Enable,
                       .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit,
                       .I2C_ClockSpeed = 400000,
                       .I2C_DutyCycle = I2C_DutyCycle_2,
                       .I2C_Mode = I2C_Mode_I2C,
                   });
    I2C_Cmd(I2C1, ENABLE);
}

err_t i2c_write(uint8_t address, const uint8_t *data, const size_t len)
{
    ERR_RETURN(i2c_write_begin(address, data));
    if (len > 1)
        ERR_RETURN(i2c_write_continue(data + 1, len - 1));
    ERR_RETURN(i2c_write_end());
    return OK;
}

err_t i2c_write_begin(uint8_t address, const uint8_t *first_byte)
{
    ERR_RETURN(i2c_wait_flag(I2C_FLAG_BUSY, RESET));

    I2C_GenerateSTART(I2C1, ENABLE);
    ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT)); // EVT5

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);
    ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // EVT6

    if (first_byte != NULL)
        I2C_SendData(I2C1, *first_byte); // EVT8_1

    return OK;
}

err_t i2c_write_continue(const uint8_t *data, const size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // EVT8
        I2C_SendData(I2C1, data[i]);
    }
    return OK;
}

err_t i2c_write_end(void)
{
    ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // EVT8_2
    I2C_GenerateSTOP(I2C1, ENABLE);
    return OK;
}

err_t i2c_read(uint8_t address, uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len)
{
    ERR_RETURN(i2c_wait_flag(I2C_FLAG_BUSY, RESET));

    if (write_data != NULL && write_len > 0)
    {
        I2C_GenerateSTART(I2C1, ENABLE);
        ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT)); // EVT5

        I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);
        ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // EVT6

        size_t i = 0;
        I2C_SendData(I2C1, write_data[i++]); // EVT8_1
        while (i < write_len)
        {
            ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // EVT8
            I2C_SendData(I2C1, write_data[i++]);
        }

        ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // EVT8_2
    }

    I2C_GenerateSTART(I2C1, ENABLE);
    ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT)); // EVT5

    if (read_len == 2)
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Next);
    else
        I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);

    I2C_AcknowledgeConfig(I2C1, ENABLE);

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);
    ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // EVT6

    if (read_len == 1)
    {
        I2C_AcknowledgeConfig(I2C1, DISABLE); // EVT6_1
        I2C_GenerateSTOP(I2C1, ENABLE);
        ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)); // EVT7
        *read_data = I2C_ReceiveData(I2C1);
    }
    else if (read_len == 2)
    {
        // Thank you WCH for not properly documenting the 2-byte read sequence
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        ERR_RETURN(i2c_wait_flag(I2C_FLAG_RXNE | I2C_FLAG_BTF, SET));
        I2C_GenerateSTOP(I2C1, ENABLE);
        *read_data = I2C_ReceiveData(I2C1);
        *(read_data + 1) = I2C_ReceiveData(I2C1);
    }
    else
        for (size_t i = 0; i < read_len; i++)
        {
            ERR_RETURN(i2c_wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)); // EVT7
            if (i + 2 == read_len)
            {
                I2C_AcknowledgeConfig(I2C1, DISABLE); // EVT7_1
                I2C_GenerateSTOP(I2C1, ENABLE);
            }
            *read_data++ = I2C_ReceiveData(I2C1);
        }

    return OK;
}
