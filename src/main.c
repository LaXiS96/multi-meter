#include <ch32v00x.h>

// TODO minimal printf implementation wired into SDI print
extern int _write(int fd, char *buf, int size);

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define SSD1306_CONTROL_COMMANDS 0x00
#define SSD1306_CONTROL_DATA 0x40

#define SSD1306_ADDRESSING_MODE 0x20
#define SSD1306_COLUMN_ADDRESS 0x21
#define SSD1306_PAGE_ADDRESS 0x22
#define SSD1306_CONTRAST 0x81
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_SEG_REMAP_DISABLE 0xA0
#define SSD1306_SEG_REMAP_ENABLE 0xA1
#define SSD1306_ENTIRE_ON_DISABLE 0xA4
#define SSD1306_ENTIRE_ON_ENABLE 0xA5
#define SSD1306_INVERT_DISABLE 0xA6
#define SSD1306_INVERT_ENABLE 0xA7
#define SSD1306_MULTIPLEX_RATIO 0xA8
#define SSD1306_DISPLAY_OFF 0xAE
#define SSD1306_DISPLAY_ON 0xAF
#define SSD1306_COM_SCAN_INCREMENT 0xC0
#define SSD1306_COM_SCAN_DECREMENT 0xC8
#define SSD1306_OFFSET 0xD3
#define SSD1306_CLOCK_OSC 0xD5
#define SSD1306_COM_PINS 0xDA
#define SSD1306_PRECHARGE 0xD9
#define SSD1306_NOP 0xE3

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
                       .I2C_ClockSpeed = 100000,
                       .I2C_DutyCycle = I2C_DutyCycle_16_9, // TODO ?
                       .I2C_Mode = I2C_Mode_I2C,
                   });
    I2C_Cmd(I2C1, ENABLE);
}

/**
 * @param address 7bit address, LSB-aligned
 * @param read read = true, write = false
 */
void i2c_send(uint8_t address, bool read, uint8_t *data, size_t len)
{
    // TODO consider DMA or interrupts
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET)
        ;

    I2C_GenerateSTART(I2C1, ENABLE);

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        ;

    I2C_Send7bitAddress(I2C1, address << 1, read ? I2C_Direction_Receiver : I2C_Direction_Transmitter);

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
    i2c_send(OLED_ADDR, false, init, sizeof(init));

    // Page: 8 pixel wide horizontal row = 1byte (LSB on top); column: 1 pixel wide vertical column
    uint8_t data[] = {
        SSD1306_CONTROL_COMMANDS,
        SSD1306_PAGE_ADDRESS,
        0,
        OLED_HEIGHT / 8 - 1,
        SSD1306_COLUMN_ADDRESS,
        0,
        OLED_WIDTH - 1,
        SSD1306_CONTROL_DATA,
        0xFF,
    };
    i2c_send(OLED_ADDR, false, data, sizeof(data));

    while (1)
    {
        _write(1, "Test\n", 5);
        Delay_Ms(1000);
    }
}
