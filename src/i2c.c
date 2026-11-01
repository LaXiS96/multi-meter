#include <ch32v00x.h>

#include "i2c.h"

// TODO if i2c device is disconnected or does not respond, the flow hangs. maybe add timeouts in while loops
// TODO consider DMA or interrupts

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

void i2c_write(uint8_t address, const uint8_t *data, const size_t len)
{
    i2c_write_begin(address);
    i2c_write_continue(data, len);
    i2c_write_end();
}

void i2c_write_begin(uint8_t address)
{
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
        ;

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;
}

void i2c_write_continue(const uint8_t *data, const size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != SET)
            ;
        I2C_SendData(I2C1, data[i]);
    }
}

void i2c_write_end(void)
{
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        ;
    I2C_GenerateSTOP(I2C1, ENABLE);
}

void i2c_read(uint8_t address, uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len)
{
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
        ;

    if (write_data != NULL && write_len > 0)
    {
        I2C_GenerateSTART(I2C1, ENABLE);

        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
            ;
        I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);

        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
            ;

        for (size_t i = 0; i < write_len; i++)
        {
            while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != SET)
                ;
            I2C_SendData(I2C1, write_data[i]);
        }

        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
            ;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;
    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        ;

    I2C_AcknowledgeConfig(I2C1, ENABLE);
    for (size_t i = 0; i < read_len; i++)
    {
        while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) != SET)
            ;
        *(read_data + i) = I2C_ReceiveData(I2C1);

        if (i + 2 == read_len)
            I2C_AcknowledgeConfig(I2C1, DISABLE);
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
}
