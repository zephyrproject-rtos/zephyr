.. zephyr:board:: tdisplay_s3_amoled_plus

Overview
********

The LilyGO `T-Display S3 AMOLED Plus`_ is an ESP32-S3 development board with a 1.91 inch AMOLED
touch display, a battery charger, a real-time clock and a microSD card slot.

Hardware
********

- ESP32-S3R8 SoC

  - Dual core Xtensa LX7 up to 240 MHz
  - 8 MB octal PSRAM
  - Wi-Fi 802.11 b/g/n
  - Bluetooth LE 5.0

- 16 MB quad SPI flash
- 1.91 inch 240x536 AMOLED display with a Raydium RM67162 controller on a 4-wire SPI interface
- Hynitron CST816T capacitive touch controller
- TI BQ25896 single cell battery charger (some boards use the compatible Silergy SY6970)
- NXP PCF85063A real-time clock
- microSD card slot on SPI
- Battery voltage measurement on IO4 through a 100k/100k resistor divider
- BOOT button on IO0 and a RESET button
- USB-C for power, flashing, logging and debugging through the built-in USB JTAG
- Two 14-pin expansion headers, and 4-pin I2C and UART connectors

.. include:: ../../../espressif/common/soc-esp32s3-features.rst
   :start-after: espressif-soc-esp32s3-features

Supported Features
==================

.. zephyr:board-supported-hw::

The display is configured in landscape orientation (536x240) by default. Touch coordinates are
reported in the native portrait orientation of the panel (240x536) and are rotated by consumers such
as LVGL according to the display orientation. The touch panel extends past the display, where a
home button is printed on the cover glass. Touches on the home button are reported outside the
display area.

The battery charger is configured for a 512 mA fast charge current and a 4.208 V charge voltage.
Boards fitted with an SY6970 charger need an overlay that replaces the ``charger`` node with a
``silergy,sy6970`` node at I2C address ``0x6a``.

Connections and IOs
===================

+---------+------------------------------------------+
| GPIO    | Function                                 |
+=========+==========================================+
| IO0     | BOOT button                              |
+---------+------------------------------------------+
| IO2     | I2C SCL (touch, charger, RTC, connector) |
+---------+------------------------------------------+
| IO3     | I2C SDA (touch, charger, RTC, connector) |
+---------+------------------------------------------+
| IO4     | Battery voltage (ADC1 channel 3)         |
+---------+------------------------------------------+
| IO6     | Display CS                               |
+---------+------------------------------------------+
| IO7     | Display D/C                              |
+---------+------------------------------------------+
| IO9     | Display TE                               |
+---------+------------------------------------------+
| IO11    | microSD CS                               |
+---------+------------------------------------------+
| IO12    | microSD MOSI                             |
+---------+------------------------------------------+
| IO13    | microSD MISO                             |
+---------+------------------------------------------+
| IO14    | microSD SCLK                             |
+---------+------------------------------------------+
| IO15    | RTC interrupt                            |
+---------+------------------------------------------+
| IO17    | Display reset                            |
+---------+------------------------------------------+
| IO18    | Display MOSI                             |
+---------+------------------------------------------+
| IO21    | Touch interrupt                          |
+---------+------------------------------------------+
| IO38    | AMOLED power supply enable               |
+---------+------------------------------------------+
| IO43    | UART0 TX                                 |
+---------+------------------------------------------+
| IO44    | UART0 RX                                 |
+---------+------------------------------------------+
| IO47    | Display SCLK                             |
+---------+------------------------------------------+

System Requirements
*******************

.. include:: ../../../espressif/common/system-requirements.rst
   :start-after: espressif-system-requirements

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

.. include:: ../../../espressif/common/building-flashing.rst
   :start-after: espressif-building-flashing

The board enters the boot ROM download mode when the BOOT button is held while resetting or
powering the device.

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
=========

.. include:: ../../../espressif/common/openocd-debugging.rst
   :start-after: espressif-openocd-debugging

References
**********

.. target-notes::

.. _`T-Display S3 AMOLED Plus`: https://lilygo.cc/products/t-display-s3-amoled-plus
