#include <ch32v00x.h>

#include "bits.h"
#include "ssd1306.h"
#include "ina226.h"

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define INA_ADDR 0x40
#define INA_SHUNT_UOHM 100000

// TODO if i2c device is disconnected or does not respond, the flow hangs. maybe add timeouts in while loops

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

    // INA226
    // 0.1Ohm shunt -> 25uA resolution
    uint8_t buf[8] = {0};
    i2c_read(INA_ADDR, (uint8_t[]){INA226_MANUFACTURER_ID}, 1, buf, 2);

    // Reset
    i2c_write(INA_ADDR, (uint8_t[]){INA226_CONFIGURATION, 0x80, 0x00}, 3);

    // Note: no point in going lower than 25uA LSB since that's the ADC max resolution
    // CAL = 0.00512 / (uA * 0.1Ohm)
    // Current_LSB = 25uA = 819.2mA / 2^15, CAL = 2048
    // Current_LSB = 50uA = 1.6384A / 2^15, CAL = 1024
    // Current_LSB = 100uA = 3.2768A / 2^15, CAL = 512
    // Current_LSB = 200uA = 6.5526A / 2^15, CAL = 256
    uint32_t current_lsb = 25; // uA
    uint32_t cal = (5120 * 1000000ULL) / ((uint64_t)current_lsb * INA_SHUNT_UOHM);
    i2c_write(INA_ADDR, (uint8_t[]){INA226_CALIBRATION, u16be(cal)}, 3);

    // TODO use interrupt alert on cnvr

    i2c_read(INA_ADDR, (uint8_t[]){INA226_CONFIGURATION}, 1, buf, 2);
    uint16_t config = ((uint16_t)buf[0] << 8) | buf[1];
    // Conversion time = n_avg * (t_vbus + t_vshunt)
    // 64 avg * (1.1ms + 0.14ms) = ~80ms
    // Effective resolution with averaging = Current_LSB / sqrt(n_avg)
    // 16 -> 6.26uA
    // 64 -> 3.125uA
    // 256 -> 1.5625uA
    // 512 -> 1.1uA
    // 1024 -> 0.78uA
    bsm(config, 9, 0b111, INA226_AVERAGE_64);
    bsm(config, 6, 0b111, INA226_CONVTIME_1100us); // Vbus
    bsm(config, 3, 0b111, INA226_CONVTIME_332us);  // Vshunt
    i2c_write(INA_ADDR, (uint8_t[]){INA226_CONFIGURATION, u16be(config)}, 3);

    while (1)
    {
        i2c_read(INA_ADDR, (uint8_t[]){INA226_BUS_VOLTAGE}, 1, buf, 2);
        uint32_t vbus = u8to16(buf[0], buf[1]) * 125 / 100; // 1.25mV LSB
        i2c_read(INA_ADDR, (uint8_t[]){INA226_CURRENT}, 1, buf, 2);
        int32_t current = u8to16(buf[0], buf[1]) * current_lsb;
        i2c_read(INA_ADDR, (uint8_t[]){INA226_POWER}, 1, buf, 2);
        uint32_t power = u8to16(buf[0], buf[1]) * current_lsb * 25; // Power_LSB = Current_LSB * 25

        printf("%lumV %lduA %luuW\n", vbus, current, power);

        Delay_Ms(100);
    }
}
