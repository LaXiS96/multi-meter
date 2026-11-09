#include <ch32v00x.h>
#include <mini-printf.h>

#include "bits.h"
#include "i2c.h"
#include "ssd1306.h"
#include "ina226.h"
#include "font.h"
#include "time.h"

void low_power_init(void)
{
    // TODO sdk example also disabled sdi
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    GPIO_InitTypeDef init = {
        .GPIO_Pin = GPIO_Pin_All,
        .GPIO_Mode = GPIO_Mode_IPD,
    };
    GPIO_Init(GPIOA, &init);
    GPIO_Init(GPIOC, &init);
    GPIO_Init(GPIOD, &init);
}

void awu_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_LSICmd(ENABLE);

    // EXTI line 9 is connected to AWU
    EXTI_Init(&(EXTI_InitTypeDef){
        .EXTI_Line = EXTI_Line9,
        .EXTI_Mode = EXTI_Mode_Event,
        .EXTI_Trigger = EXTI_Trigger_Falling,
        .EXTI_LineCmd = ENABLE,
    });

    // prescaler 512, window 36 -> 148ms
    PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_512);
    PWR_AWU_SetWindowValue(36);

    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET)
        ;
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    SDI_Printf_Enable();

    time_init();

    low_power_init();
    awu_init();

    // PC2 = SCL, PC1 = SDA
    i2c_init();

    ssd1306_init();

    // TODO read shunt voltage instead of current and power registers, should be faster too in terms of i2c transaction time vs calculation
    ina226_init(25);
    ina226_configure(INA226_AVERAGE_64, INA226_CONVTIME_1100us, INA226_CONVTIME_1100us);

    uint8_t left_advance = font_get_text_advance(&font, "-") + font_get_max_advance(&font, '0', '9') * 3;
    uint16_t advance = 0;

    char str[16] = {0};
    while (1)
    {
        ssd1306_blank();

        ina226_flag_t flags = 0;
        ERR_GOTO(ina226_read_flags(&flags), error);
        if (flags & INA226_FLAG_CONVERSION_READY)
        {
            uint32_t vbus = 0;
            ERR_GOTO(ina226_read_bus_voltage(&vbus), error);
            uint32_t vbus_int = vbus / 1000;
            uint32_t vbus_dec = vbus % 1000;

            int32_t current = 0;
            ERR_GOTO(ina226_read_current(&current), error);
            char *current_sign = current < 0 ? "-" : "";
            uint32_t current_int = u32abs(current) / 1000;
            uint32_t current_dec = u32abs(current) % 1000;

            // uint32_t power = ina226_read_power();
            // int32_t power_int = power > 1000000 ? power / 1000000 : power / 1000;
            // int32_t power_dec = power > 1000000 ? power % 1000000 : power % 1000;
            // char *power_uom = power > 1000000 ? "W" : "mW";

            advance = font_get_text_advance(&font, "V");
            ssd1306_print(&font, 0, SSD1306_WIDTH - advance, "V");

            mini_snprintf(str, sizeof(str), "%lu", vbus_int);
            advance = font_get_text_advance(&font, str);
            ssd1306_print(&font, 0, left_advance - advance, str);
            mini_snprintf(str, sizeof(str), ".%03lu", vbus_dec);
            ssd1306_print(&font, 0, left_advance, str);

            advance = font_get_text_advance(&font, "mA");
            ssd1306_print(&font, 2, SSD1306_WIDTH - advance, "mA");

            mini_snprintf(str, sizeof(str), "%s%ld", current_sign, current_int);
            advance = font_get_text_advance(&font, str);
            ssd1306_print(&font, 2, left_advance - advance, str);
            mini_snprintf(str, sizeof(str), ".%03ld", current_dec);
            ssd1306_print(&font, 2, left_advance, str);

            // mini_snprintf(str, sizeof(str), "%lu.%03lu %s", power_int, power_dec, power_uom);
            // ssd1306_text(str, 2, 0);
        }
        else
        {
        error:
            // TODO move mapping to errors.h
            const char *err_str = "";
            switch (err)
            {
            case I2C_TIMEOUT:
                err_str = "I2C_TIMEOUT";
                break;
            case I2C_NACK:
                err_str = "I2C_NACK";
                break;
            case I2C_BUS_ERROR:
                err_str = "I2C_BUS_ERROR";
                break;
            case I2C_ARBITRATION_LOST:
                err_str = "I2C_ARBITRATION_LOST";
                break;
            case I2C_OVERRUN:
                err_str = "I2C_OVERRUN";
                break;
            default:
                mini_snprintf(str, sizeof(str), "0x%X", flags);
                err_str = str;
            }
            ssd1306_text(err_str, 0, 0);
        }

        ssd1306_flush();

        PWR_AutoWakeUpCmd(ENABLE);
        PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);
    }
}
