.. zephyr:board:: flipper_zero

Overview
********

The Flipper Zero is a portable multi-tool for hardware exploration, built
around an STM32WB55RG microcontroller with a 128x64 monochrome display, a
five-way D-pad with a back button, an RGB LED and a Sub-GHz, NFC, RFID,
infrared and Bluetooth LE radio set. See the `Flipper Zero documentation`_ for
more details.

Hardware
********

- `STM32WB55RG`_ dual-core microcontroller: Arm Cortex-M4 application core up
  to 64 MHz and Arm Cortex-M0+ radio core running ST's wireless stack
- 1 MiB flash (the upper part is reserved for the radio stack) and 256 KiB RAM
- 128x64 monochrome Sitronix ST7567 LCD on SPI2
- microSD card slot with card detect, sharing SPI2 with the display
- Six push buttons: Up, Down, Left, Right, OK and Back
- TI LP5562 LED driver on I2C1, driving the RGB LED and the display backlight
- Speaker driven by a PWM output
- TI BQ25896 battery charger on I2C1 for the 2100 mAh battery
- USART1 on pins 13 (TX) and 14 (RX) of the GPIO header
- 18-pin GPIO header
- USB 2.0 full-speed device port
- 32 MHz (HSE) and 32.768 kHz (LSE) crystals

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

.. list-table::
   :header-rows: 1

   * - Function
     - Pins
   * - SPI2 SCK, MOSI (display and SD card)
     - PD1, PB15
   * - Display CS
     - PC11
   * - Display D/C, reset
     - PB1, PB0
   * - Buttons Up, Down, Left, Right
     - PB10, PC6, PB11, PB12
   * - Buttons OK, Back
     - PH3, PC13
   * - USART1 TX, RX
     - PB6, PB7
   * - SD card MISO, CS, card detect
     - PC2, PC12, PC10
   * - I2C1 SCL, SDA
     - PA9, PA10
   * - USB D-, D+
     - PA11, PA12
   * - Speaker (TIM16 CH1)
     - PB8

GPIO Header
===========

The 18-pin GPIO header is described by the ``flipper_header`` connector node.
The parent pin index is the header pin number printed on the device, and the
power and ground pins are not mapped. See :dtcompatible:`flipper,gpio-header`
for the complete pin list.

The 5 V supply on pin 1 is driven by the USB OTG boost converter of the BQ25896
when no USB host is connected. It is described by the ``vbus_otg`` regulator
node, which is off at boot and can be enabled through the regulator API.

System Clock
============

The system clock is driven directly by the 32 MHz external crystal (HSE). The
32.768 kHz crystal (LSE) clocks the RTC and LPTIM1, which is used as the system
timer.

Serial Port
===========

The Zephyr console and shell are assigned to a USB CDC ACM virtual serial port
on the USB-C connector, so no serial adapter is needed. The port appears once
the application has started and the host has enumerated the device. Output
printed before the host opens the port can be lost.

USART1 (115200 8N1) is available on pins 13 (TX) and 14 (RX) of the GPIO
header and needs a 3.3 V USB to serial adapter. To use it as the console, set
:kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` to ``n`` and point the
``zephyr,console`` and ``zephyr,shell-uart`` chosen properties to ``&usart1``
in a devicetree overlay.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Zephyr replaces the stock firmware, which occupies the whole flash starting at
``0x08000000`` and has no separate bootloader. The Zephyr image is written to
the same address.

The default runner is ``dfu-util``, which programs the board through the STM32
ROM DFU bootloader over USB:

#. Hold the ``Left``, ``Back`` and ``OK`` buttons for a couple of seconds, then
   release ``Left`` and ``Back`` while keeping ``OK`` pressed. The device
   enumerates as ``0483:df11`` and the display stays dark.
#. Build and flash an application, for example
   :zephyr:code-sample:`character-frame-buffer`:

   .. zephyr-app-commands::
      :zephyr-app: samples/subsys/display/cfb
      :board: flipper_zero
      :goals: build flash
      :compact:

The application starts after the flash completes. The ROM bootloader cannot be
overwritten, so the device can always be recovered through the same button
combination.

.. warning::

   The stock firmware is erased. It can be restored by flashing its ``.dfu``
   image with ``dfu-util``, or with the build system of the
   `Flipper Zero firmware`_.

Debugging
=========

The board has no on-board debug probe and no debug runner is configured. The
SWD signals are available on pins 10 (SWCLK) and 12 (SWDIO) of the GPIO header.

References
**********

.. target-notes::

.. _Flipper Zero documentation:
   https://docs.flipper.net/zero

.. _Flipper Zero firmware:
   https://github.com/flipperdevices/flipperzero-firmware

.. _STM32WB55RG:
   https://www.st.com/en/microcontrollers-microprocessors/stm32wb55rg.html
