LVGL Accelerometer Chart - GC9D01 160x160 Round LCD
====================================================

Overview
********

Real-time accelerometer chart using **LVGL** on **Zephyr RTOS** with an
**NXP FXOS8700CQ** sensor on a **Waveshare 0.71inch Round LCD Module** driven
by a **GC9D01** controller via SPI.

Chart series use color-coded lines: X=red (top-left), Y=blue (top-center), Z=green (top-right).
The display is round, so the labels are inset from the edges to stay within the visible area.

Display
*******

* **Product**: `Waveshare 0.71inch Round LCD Display Module <https://www.waveshare.com/0.71inch-lcd-module.htm>`_
* **Controller**: GC9D01N (``galaxycore,gc9d01``) — custom out-of-tree driver in ``custom_driver_module/``
* **Resolution**: 160×160 pixels (round IPS, 65K colors)
* **Display size**: 18×18 mm, module 20.12×22.3 mm
* **Interface**: 4-wire SPI via MIPI DBI
* **Connector**: SH1.0 8-pin cable
* **Supply**: 3.3 V / 5 V

.. note::
   The Zephyr built-in ``galaxycore,gc9x01x`` driver uses the GC9A01 init sequence,
   which is incompatible with the GC9D01N. The custom driver in ``custom_driver_module/``
   implements the correct BOE panel initialization sequence.

Hardware
********

* **MCU**: nRF52832 (custom board ``bruno_nrf52832``)
* **Sensor**: NXP FXOS8700CQ on I2C0 (address 0x1E)
* **Display**: Waveshare 0.71inch Round LCD (GC9D01) on SPI1

Pinout
------

Display SH1.0 → Bruno nRF52832
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+--------+----------+-----------+
| Module | Signal   | nRF52832  |
+========+==========+===========+
| VCC    | Power    | 3.3 V     |
+--------+----------+-----------+
| GND    | Ground   | GND       |
+--------+----------+-----------+
| DIN    | MOSI     | P0.12     |
+--------+----------+-----------+
| CLK    | SCK      | P0.11     |
+--------+----------+-----------+
| CS     | Chip Sel | P0.19     |
+--------+----------+-----------+
| DC     | Data/Cmd | P0.20     |
+--------+----------+-----------+
| RST    | Reset    | P0.22     |
+--------+----------+-----------+
| BL     | Backlight| (3.3 V)   |
+--------+----------+-----------+

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
