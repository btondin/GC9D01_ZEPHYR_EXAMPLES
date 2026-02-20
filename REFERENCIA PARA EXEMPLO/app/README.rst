LVGL Accelerometer Chart - SSD1331 96x64 RGB OLED
===================================================

Overview
********

Real-time accelerometer chart using **LVGL** on **Zephyr RTOS** with an
**NXP FXOS8700CQ** sensor on a generic **96x64 RGB OLED** module driven
by an **SSD1331** controller via SPI.

Display
*******

* **Controller**: SSD1331 (``solomon,ssd1331``)
* **Resolution**: 96x64
* **Color**: RGB 16-bit
* **Interface**: SPI via MIPI DBI

Chart series use color-coded lines: X=red, Y=blue, Z=green.

Hardware
********

* **MCU**: nRF52832 (custom board ``bruno_nrf52832``)
* **Sensor**: NXP FXOS8700CQ on I2C0 (address 0x1E)
* **Display**: SSD1331 96x64 RGB OLED module on SPI1

Pinout
------

* SCK: P0.11
* MOSI: P0.12
* CS: P0.19
* DC: P0.20
* RESET: P0.22

Building and Flashing
*********************

::

    west build -b bruno_nrf52832/nrf52832
    west flash

Other Displays
**************

This repository has one branch per display configuration.
See all available branches at the
`repository page <https://github.com/btondin/LVGL_AULA>`_.

License
*******

Apache-2.0
