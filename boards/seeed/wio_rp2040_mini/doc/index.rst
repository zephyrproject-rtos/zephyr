.. zephyr:board:: wio_rp2040_mini

Overview
********

The Wio RP2040 Mini Dev Board is a compact IoT microcontroller based on the Raspberry Pi
RP2040 microcontroller from seeed studio.

The RP2040 has onboard ESP8285 Wi-Fi Chip, its built-in USB bootloader allows to flash
firmware via drag-and-drop method.

Hardware
********

* Dual-core Arm Cortex-M0+ processor running up to 133 MHz
* 264 KiB on-chip SRAM
* 2 MiB on-board QSPI flash
* 1 user LED connected to GPIO13
* USB 1.1 device controller
* 2 UART peripherals
* 2 I2C controllers
* 2 SPI controllers
* 4 ADC channels
* 1 watchdog timer
* RTC

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The RP2040 GPIOs can be routed to multiple peripherals.
The configuration of these routes can be modified through DTS.
Please refer to the datasheet to see the possible routings for
each peripheral.
The following mappings are configured for the Wio RP2040 Mini:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - GPIO
     - Pinctrl symbol
   * - UART0 TX
     - P0
     - ``UART0_TX_P0``
   * - UART0 RX
     - P1
     - ``UART0_RX_P1``
   * - I2C1 SDA
     - P2
     - ``I2C1_SDA_P2``
   * - I2C1 SCL
     - P3
     - ``I2C1_SCL_P3``
   * - I2C0 SDA
     - P4
     - ``I2C0_SDA_P4``
   * - I2C0 SCL
     - P5
     - ``I2C0_SCL_P5``
   * - SPI0 RX
     - P16
     - ``SPI0_RX_P16``
   * - SPI0 CSn
     - P17
     - ``SPI0_CSN_P17``
   * - SPI0 SCK
     - P18
     - ``SPI0_SCK_P18``
   * - SPI0 TX
     - P19
     - ``SPI0_TX_P19``
   * - User LED
     - P13
     - N/A
   * - ADC channel 0
     - P26
     - ``ADC_CH0_P26``
   * - ADC channel 1
     - P27
     - ``ADC_CH1_P27``
   * - ADC channel 2
     - P28
     - ``ADC_CH2_P28``
   * - ADC channel 3
     - P29
     - ``ADC_CH3_P29``

UART0
-----

UART0 is the default console and shell UART. It operates at 115200 baud.

I2C
---

I2C0 is enabled at the standard 100 kHz bit rate. I2C1 is enabled at the fast
400 kHz bit rate.

SPI
---

SPI0 is enabled at 8 MHz.

LED
---

The user LED is available through the ``led0`` alias and is connected to GPIO13
as an active-high output.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications can be built and flashed in the usual Zephyr way. For example,
build and flash the Hello World sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world/
   :board: wio_rp2040_mini
   :goals: build flash
   :compact:

UF2
===

The board configuration generates a UF2 image. Enter the RP2040 USB bootloader,
then copy or flash the generated ``build/zephyr/zephyr.uf2`` file to the
mass-storage device exposed by the board.

References
**********

.. target-notes::

.. _Seeed Wio RP2040 Mini:
   https://wiki.seeedstudio.com/Wio_RP2040_mini_Dev_Board-Onboard_Wifi/
