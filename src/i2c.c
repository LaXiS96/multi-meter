#include <ch32v00x.h>

#include "i2c.h"

// TODO if i2c device is disconnected or does not respond, the flow hangs. maybe add timeouts in while loops

typedef enum
{
    /** No transaction in progress */
    STATE_IDLE,
    /** Start condition pending */
    STATE_START,
    /** Slave address transmission pending */
    STATE_ADDRESS,
    /** DMA transmission pending */
    STATE_TX_DMA,
    /** Last byte transmission pending */
    STATE_TX_LAST,
    STATE_RESTART,
    STATE_RX_DMA,
    STATE_RX_LAST,
    STATE_ERROR,
} i2c_state_t;

static struct
{
    volatile i2c_state_t state;
    uint8_t address;
    uint8_t *read_buffer;
    size_t read_len;
} trans = {0};

void i2c_init(bool enable_dma)
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

    if (enable_dma)
    {
        // TODO priorities
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        NVIC_Init(&(NVIC_InitTypeDef){
            .NVIC_IRQChannel = DMA1_Channel6_IRQn,
            .NVIC_IRQChannelPreemptionPriority = 0,
            .NVIC_IRQChannelSubPriority = 1,
            .NVIC_IRQChannelCmd = ENABLE,
        });
        NVIC_Init(&(NVIC_InitTypeDef){
            .NVIC_IRQChannel = I2C1_EV_IRQn,
            .NVIC_IRQChannelPreemptionPriority = 0,
            .NVIC_IRQChannelSubPriority = 1,
            .NVIC_IRQChannelCmd = ENABLE,
        });
        NVIC_Init(&(NVIC_InitTypeDef){
            .NVIC_IRQChannel = I2C1_ER_IRQn,
            .NVIC_IRQChannelPreemptionPriority = 0,
            .NVIC_IRQChannelSubPriority = 1,
            .NVIC_IRQChannelCmd = ENABLE,
        });
        I2C_DMACmd(I2C1, ENABLE);
    }
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

bool i2c_transact_dma(uint8_t address, const uint8_t *write_data, const size_t write_len, uint8_t *read_data, const size_t read_len)
{
    // TODO better error return values
    if (trans.state != STATE_IDLE)
        return false;

    trans.address = address;
    trans.read_buffer = read_data;
    trans.read_len = read_len;

    if (write_data != NULL)
    {
        // DMA1_Channel6 is I2C1_TX
        DMA_DeInit(DMA1_Channel6);
        DMA_Init(DMA1_Channel6, &(DMA_InitTypeDef){
                                    .DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DATAR,
                                    .DMA_MemoryBaseAddr = (uint32_t)write_data,
                                    .DMA_DIR = DMA_DIR_PeripheralDST,
                                    .DMA_BufferSize = write_len,
                                    .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
                                    .DMA_MemoryInc = DMA_MemoryInc_Enable,
                                    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
                                    .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
                                    .DMA_Mode = DMA_Mode_Normal,
                                    .DMA_Priority = DMA_Priority_VeryHigh,
                                    .DMA_M2M = DMA_M2M_Disable,
                                });

        I2C_ITConfig(I2C1, I2C_IT_EVT /*| I2C_IT_ERR*/, ENABLE);
        // TODO which interrupts to enable? also disable them when transaction completes
        DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);
    }

    if (read_data != NULL)
    {
        // TODO rx dma setup
    }

    // TODO skip unneeded transaction states for read/write

    // TODO timeout
    // Wait for the bus to be free (eg. the previous stop condition to complete)
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET)
        ;
    I2C_GenerateSTART(I2C1, ENABLE);
    trans.state = STATE_START;

    return true;
}

// I2C1 event interrupt
__attribute__((interrupt("WCH-Interrupt-fast"))) void I2C1_EV_IRQHandler(void)
{
    if (I2C_GetITStatus(I2C1, I2C_IT_SB) == SET)
    {
        // Start condition has been generated on the bus
        if (trans.state == STATE_START)
        {
            I2C_Send7bitAddress(I2C1, trans.address << 1, I2C_Direction_Transmitter);
            trans.state = STATE_ADDRESS;
            // Interrupt is cleared by reading status and writing address
        }
        // TODO if restart send read address
    }
    else if (I2C_GetITStatus(I2C1, I2C_IT_ADDR) == SET)
    {
        // Slave address has been sent on the bus
        if (trans.state == STATE_ADDRESS)
        {
            DMA_Cmd(DMA1_Channel6, ENABLE);
            trans.state = STATE_TX_DMA;
            // Interrupt is cleared by reading status 1 and 2 registers
            I2C_ReadRegister(I2C1, I2C_Register_STAR2);
        }
        // TODO after restart, enable read dma
        // TODO when reading, use I2C1_CTLR2.LAST to set NACK (also remember to set ACK initially)
    }
    else if (I2C_GetITStatus(I2C1, I2C_IT_BTF) == SET)
    {
        // Byte Transfer Finished, because no new data was written to data register
        if (trans.state == STATE_TX_LAST)
        {
            // TODO send restart condition and later start dma read, if requested
            // TODO on stop, both tx-only and tx+rx, clean up and disable interrupts/dma
            I2C_GenerateSTOP(I2C1, ENABLE);
            trans.state = STATE_IDLE;
            // Interrupt will be cleared by start/stop condition
        }
    }
}

// I2C1 error interrupt
__attribute__((interrupt("WCH-Interrupt-fast"))) void I2C1_ER_IRQHandler(void)
{
    // TODO
}

// DMA I2C TX interrupt
__attribute__((interrupt("WCH-Interrupt-fast"))) void DMA1_Channel6_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC6) == SET)
    {
        // DMA transmission completed
        DMA_Cmd(DMA1_Channel6, DISABLE);
        trans.state = STATE_TX_LAST;
        DMA_ClearITPendingBit(DMA1_IT_TC6);
    }
}
