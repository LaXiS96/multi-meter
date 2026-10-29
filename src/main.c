#include <ch32v00x.h>

#include "ssd1306.h"
#include "ina226.h"

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define INA_ADDR 0x40

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

/**
 * @param address 7bit address, LSB-aligned
 */
void i2c_write(uint8_t address, uint8_t *data, size_t len)
{
    // TODO consider DMA or interrupts
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
        ;

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        ;

    for (size_t i = 0; i < len; i++)
    {
        while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != SET)
            ;
        I2C_SendData(I2C1, data[i]);
    }

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

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    SDI_Printf_Enable();

    // PC2 = SCL, PC1 = SDA
    i2c_init();

    uint8_t init[] = {
        SSD1306_CONTROL_COMMANDS,
        SSD1306_DISPLAY_OFF,
        SSD1306_MULTIPLEX_RATIO,
        OLED_HEIGHT - 1,
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
    i2c_write(OLED_ADDR, init, sizeof(init));

    // Page: 8 pixel wide horizontal row = 1byte (LSB on top); column: 1 pixel wide vertical column
    uint8_t data1[] = {
        SSD1306_CONTROL_COMMANDS,
        SSD1306_PAGE_ADDRESS,
        0,
        OLED_HEIGHT / 8 - 1,
        SSD1306_COLUMN_ADDRESS,
        0,
        OLED_WIDTH - 1,
    };
    i2c_write(OLED_ADDR, data1, sizeof(data1));

    uint8_t data2[] = {
        SSD1306_CONTROL_DATA,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
    };
    i2c_write(OLED_ADDR, data2, sizeof(data2));

    // TODO display font rendering (match ssd1306 gddram format)

    // TODO ina226
    uint8_t buf[8] = {0};
    i2c_read(INA_ADDR, (uint8_t[]){INA226_MANUFACTURER_ID}, 1, buf, 2);

    while (1)
    {
        i2c_read(INA_ADDR, (uint8_t[]){INA226_BUS_VOLTAGE}, 1, buf, 2);
        uint32_t vbus = ((buf[0] << 8) | buf[1]) * 125;
        printf("bus: %lu\n", vbus);
        Delay_Ms(100);
    }
}
