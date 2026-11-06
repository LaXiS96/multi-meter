#include <ch32v00x.h>
#include <mini-printf.h>

#include "bits.h"
#include "i2c.h"
#include "ssd1306.h"
#include "ina226.h"
#include "font.h"

static volatile bool alert = false;

void exti_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_Init(GPIOD, &(GPIO_InitTypeDef){
                         .GPIO_Pin = GPIO_Pin_0,
                         .GPIO_Mode = GPIO_Mode_IPU,
                         .GPIO_Speed = GPIO_Speed_30MHz,
                     });
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource0);
    EXTI_Init(&(EXTI_InitTypeDef){
        .EXTI_Line = EXTI_Line0,
        // TODO event vs interrupt?
        .EXTI_Mode = EXTI_Mode_Interrupt,
        .EXTI_Trigger = EXTI_Trigger_Falling,
        .EXTI_LineCmd = ENABLE,
    });
    NVIC_Init(&(NVIC_InitTypeDef){
        .NVIC_IRQChannel = EXTI7_0_IRQn,
        // TODO priority?
        .NVIC_IRQChannelPreemptionPriority = 0,
        .NVIC_IRQChannelSubPriority = 1,
        .NVIC_IRQChannelCmd = ENABLE,
    });
}

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    SDI_Printf_Enable();

    // PD0 -> EXTI0
    exti_init();

    // PC2 = SCL, PC1 = SDA
    i2c_init();

    ssd1306_init();

    // TODO read shunt voltage instead of current and power registers, should be faster too
    ina226_init(25);
    ina226_alert(INA226_ALERT_ON_CONVERSION_READY);
    ina226_configure(INA226_AVERAGE_64, INA226_CONVTIME_1100us, INA226_CONVTIME_1100us);

    uint8_t left_advance = font_get_text_advance(&font, "-") + font_get_max_advance(&font, '0', '9') * 3;
    uint16_t advance = 0;

    char str[16] = {0};
    while (1)
    {
        if (alert)
        {
            uint32_t vbus = ina226_read_bus_voltage();
            uint32_t vbus_int = vbus / 1000;
            uint32_t vbus_dec = vbus % 1000;

            int32_t current = ina226_read_current();
            char *current_sign = current < 0 ? "-" : "";
            uint32_t current_int = u32abs(current) / 1000;
            uint32_t current_dec = u32abs(current) % 1000;

            uint32_t power = ina226_read_power();
            int32_t power_int = power > 1000000 ? power / 1000000 : power / 1000;
            int32_t power_dec = power > 1000000 ? power % 1000000 : power % 1000;
            char *power_uom = power > 1000000 ? "W" : "mW";

            ssd1306_blank();

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

            ssd1306_flush();

            // Read INA226 flags to reset alert/conversion ready flag
            alert = false;
            ina226_read_flags();
        }
    }
}

__attribute__((interrupt("WCH-Interrupt-fast"))) void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        alert = true;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
