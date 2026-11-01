#include <ch32v00x.h>
#include <mini-printf.h>

#include "i2c.h"
#include "ssd1306.h"
#include "ina226.h"

int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    SDI_Printf_Enable();

    // PC2 = SCL, PC1 = SDA
    i2c_init();

    // TODO display font rendering (match ssd1306 gddram format)
    ssd1306_init();
    // ssd1306_text("Hello, World!", 0, 0);
    // ssd1306_flush();

    // INA226
    // 0.1Ohm shunt -> 25uA resolution
    // uint8_t buf[8] = {0};
    // i2c_read(INA226_ADDR, (uint8_t[]){INA226_MANUFACTURER_ID}, 1, buf, 2);

    // TODO use interrupt alert on cnvr
    ina226_init();
    ina226_configure(INA226_AVERAGE_64, INA226_CONVTIME_140us, INA226_CONVTIME_1100us);

    char str[16] = {0};
    while (1)
    {
        uint32_t vbus = ina226_get_voltage();
        int32_t current = ina226_get_current();
        uint32_t power = ina226_get_power();

        ssd1306_blank();
        mini_snprintf(str, sizeof(str), "%lu mV", vbus);
        ssd1306_text(str, 0, 0);
        mini_snprintf(str, sizeof(str), "%ld uA", current);
        ssd1306_text(str, 1, 0);
        mini_snprintf(str, sizeof(str), "%lu uW", power);
        ssd1306_text(str, 2, 0);
        ssd1306_flush();

        Delay_Ms(100);
    }
}
