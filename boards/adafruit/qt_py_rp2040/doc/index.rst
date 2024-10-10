.. _adafruit_qt_py_rp2040:

Adafruit QT Py RP2040
#####################

Overview
********

The Adafruit QT Py RP2040 is a small, low-cost, versatile board from
Adafruit. It is equipped with an RP2040 SoC, an on-board RGB Neopixel,
a USB connector, and a STEMMA QT connector. The USB bootloader allows
it to be flashed without any adapter, in a drag-and-drop manner.

Hardware
********
- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 8MB on-board QSPI flash with XIP capabilities
- 11 GPIO pins
- 4 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers (one via STEMMA QT connector)
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- On-board RGB LED
- 1 Watchdog timer peripheral


.. figure:: img/qtpy_rp2040.jpg
     :align: center
     :alt: Adafruit QT Py RP2040

     Adafruit QT Py RP2040 (Image courtesy of Adafruit)

Supported Features
==================

The adafruit_qt_py_rp2040 board configuration supports the following
hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - NVIC
     - N/A
     - :dtcompatible:`arm,v6m-nvic`
   * - UART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`raspberrypi,pico-uart`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`raspberrypi,pico-gpio`
   * - ADC
     - :kconfig:option:`CONFIG_ADC`
     - :dtcompatible:`raspberrypi,pico-adc`
   * - I2C
     - :kconfig:option:`CONFIG_I2C`
     - :dtcompatible:`snps,designware-i2c`
   * - SPI
     - :kconfig:option:`CONFIG_SPI`
     - :dtcompatible:`raspberrypi,pico-spi`
   * - USB Device
     - :kconfig:option:`CONFIG_USB_DEVICE_STACK`
     - :dtcompatible:`raspberrypi,pico-usbd`
   * - HWINFO
     - :kconfig:option:`CONFIG_HWINFO`
     - N/A
   * - Watchdog Timer (WDT)
     - :kconfig:option:`CONFIG_WATCHDOG`
     - :dtcompatible:`raspberrypi,pico-watchdog`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`raspberrypi,pico-pwm`
   * - Flash
     - :kconfig:option:`CONFIG_FLASH`
     - :dtcompatible:`raspberrypi,pico-flash`
   * - UART (PIO)
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`raspberrypi,pico-uart-pio`

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART1_TX : P20
- UART1_RX : P5
- I2C0_SDA : P24
- I2C0_SCL : P25
- I2C1_SDA : P22
- I2C1_SCL : P23
- SPI0_RX : P4
- SPI0_SCK : P6
- SPI0_TX : P3

Programming and Debugging
*************************

Flashing
========

Using UF2
---------

Since it doesn't expose the SWD pins, you must flash the Adafruit QT Py RP2040 with
a UF2 file. By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the QT Py RP2040 is powered on with the ``BOOTSEL``
button pressed, it will appear on the host as a mass storage device. The
UF2 file should be drag-and-dropped to the device, which will flash the QT Py RP2040.

.. target-notes::

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _Primary Guide\: Adafruit QT Py RP2040:
  https://learn.adafruit.com/adafruit-qt-py-2040
