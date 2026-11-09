# multi-meter

Measurement tool based on the INA226 voltage/current monitor, with OLED display, using a CH32V003 microcontroller.

## Hardware

- CH32V003 WeActStudio CoreBoard
- INA226 board
    - 100mOhm shunt, 25uA resolution, +/-819.2mA range (~82mV max drop)
    - Wiring:
        - VBUS and IN+ to supply positive, IN- to load
        - Supply and load GND must be shared
        - Any voltage on IN+/IN-/VBUS must not exceed 36V referenced to GND
- [0.91" white OLED](https://www.lcdwiki.com/0.91inch_IIC_OLED_Module_SSD1306_SKU:MC091GX)
- Powered by 1/2 AA/AAA cells via TPS613221A 3.3V boost converter
    - 18mW measured at the battery, with following settings:
        - CH32V003: 8MHz HSI, standby (deep sleep) with auto wakeup every 148ms
        - INA226: 64 average 1.1ms bus 1.1ms shunt
    - Should last 100+ hours with a pair of 850mAh NiMH AAA cells (= ~2Wh)
    - In alternative: 18650 Li-ion via TPS63802 3.3V buck-boost converter, with TP4056 1A USB charger + DW01 cell protection
        - (requires voltage divider at ADC input)

## Software components and tools

- [Official SDK](https://github.com/openwch/ch32v003) from folder `EVT/EXAM/SRC`
- SVD obtained from `MounRiverStudio_Linux_X64_V2.5.0.tar.xz`
- [wlink](https://github.com/ch32-rs/wlink/releases)
- [mini-printf](https://github.com/mludvig/mini-printf)

Font generated with: https://github.com/LaXiS96/embedded-font-converter

https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

## TODO
- PA2 ADC readout for battery voltage
- interrupt/DMA based I2C transactions
- extract sdk into submodule repo, reuse in other ch32v003 projects
- remove alert connection since it's not used anymore
