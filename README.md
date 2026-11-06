# multi-meter

SDK obtained from repository `https://github.com/openwch/ch32v003` folder `EVT/EXAM/SRC`.  
SVD obtained from `MounRiverStudio_Linux_X64_V2.5.0.tar.xz`.

[`wlink`](https://github.com/ch32-rs/wlink/releases)

https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

https://github.com/mludvig/mini-printf

INA226 wiring:
- IN+ to supply positive
- IN- to load positive
- supply and load GND must be shared, and any voltage on IN+/IN-/VBUS must not exceed 36V referenced to GND

Generate fonts with: https://github.com/LaXiS96/embedded-font-converter

## TODO
- extract sdk into submodule repo, reuse in other ch32v003 projects
- hardware:
    - CH32V003
        - prefer WeAct CoreBoard since it doesn't have HSE which we don't need
    - power source
        - option 1: li-ion 18650 -> TP4056+DW01 (1A charge current) -> MCP1700
            - ~10Wh, LDO has low efficiency at high Vin-Vout difference, may not maintain 3.3V during li-ion discharge
        - option 2: li-ion 18650 -> TP4056+DW01 (1A charge current) -> TPS63802
            - ~10Wh, high efficiency, stable 3.3V until cutoff, (switching noise)
        - option 3: 1 or 2 NiMH cells (no charger/protection) -> TPS61322
            - ~2.5Wh or ~5Wh, lower energy density, needs external charger but cells can be replaced, may last longer when not in use
        - option n: li-ion 500mAh 13350 -> TP4056+DW01 (modified for lower charge current) -> ???
    - TP4056
        - B+/- to cell
        - OUT+ to power switch
        - OUT- to common GND
    - MCP1700 3.3V LDO
        - Vin from power switch
        - Vout to common 3V3
        - bypass caps
    - battery voltage monitoring
        - use ADC connected to LDO Vin, requires voltage divider
    - banana binding posts for current (with banana to test clips cable)
    - ??? for voltage
