#include <ch32v00x.h>
#include <mini-printf.h>

#include "bits.h"
#include "i2c.h"
#include "ssd1306.h"
#include "ina226.h"

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
    // TODO keep in mind that if the debugger is detached or has sdi print disabled, printf() wil hang!
    SDI_Printf_Enable();

    // PD0 -> EXTI0
    exti_init();

    // PC2 = SCL, PC1 = SDA
    i2c_init(true);

    i2c_write_dma(INA226_ADDR, (uint8_t[]){INA226_REG_MANUFACTURER_ID}, 1);

    while (1)
        ;

    // TODO display font rendering (match ssd1306 gddram format)
    // ssd1306_init();

    // // TODO read shunt voltage instead of current and power registers, should be faster too
    // ina226_init(25);
    // ina226_alert(INA226_ALERT_ON_CONVERSION_READY);
    // ina226_configure(INA226_AVERAGE_64, INA226_CONVTIME_1100us, INA226_CONVTIME_1100us);

    // char str[16] = {0};
    // while (1)
    // {
    //     if (alert)
    //     {
    //         uint32_t vbus = ina226_read_bus_voltage();
    //         uint32_t vbus_int = vbus / 1000;
    //         uint32_t vbus_dec = vbus % 1000;

    //         int32_t current = ina226_read_current();
    //         char *current_sign = current < 0 ? "-" : "";
    //         uint32_t current_int = u32abs(current) / 1000;
    //         uint32_t current_dec = u32abs(current) % 1000;

    //         uint32_t power = ina226_read_power();
    //         int32_t power_int = power > 1000000 ? power / 1000000 : power / 1000;
    //         int32_t power_dec = power > 1000000 ? power % 1000000 : power % 1000;
    //         char *power_uom = power > 1000000 ? "W" : "mW";

    //         ssd1306_blank();
    //         mini_snprintf(str, sizeof(str), "%lu.%03lu V", vbus_int, vbus_dec);
    //         ssd1306_text(str, 0, 0);
    //         mini_snprintf(str, sizeof(str), "%s%ld.%03ld mA", current_sign, current_int, current_dec);
    //         ssd1306_text(str, 1, 0);
    //         mini_snprintf(str, sizeof(str), "%lu.%03lu %s", power_int, power_dec, power_uom);
    //         ssd1306_text(str, 2, 0);
    //         ssd1306_flush();

    //         // Read INA226 flags to reset alert/conversion ready flag
    //         alert = false;
    //         ina226_read_flags();
    //     }
    // }
}

__attribute__((interrupt("WCH-Interrupt-fast"))) void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        alert = true;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
