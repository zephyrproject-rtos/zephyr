.. zephyr:board:: adafruit_feather_adalogger_rp2040

Overview
********

The `Adafruit Feather Adalogger RP2040`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd. The board has a MicroSD-card socket
for storing data, and a Stemma QT connector for easy sensor usage.
It is compatible with the Feather board form factor, and has
a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 17 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- Red LED
- RGB LED (Neopixel)
- Stemma QT I2C connector
- MicroSD-card holder
- Built-in lithium ion battery charger


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - A0 ADC0 : GPIO26
    - A1 ADC1 : GPIO27
    - A2 ADC2 : GPIO28
    - A3 ADC3 : GPIO29
    - D24 : GPIO24
    - D25 : GPIO25
    - SCK SPI1 SCK : GPIO14
    - MO SPI1 MOSI : GPIO15
    - MI SPI1 MISO : GPIO8
    - RX UART0 : GPIO1
    - TX UART0 : GPIO0
    - D4 : GPIO4
    - SDA I2C1 : GPIO2
    - SCL I2C1 : GPIO3
    - D5 : GPIO5
    - D6 : GPIO6
    - D9 : GPIO9
    - D10 : GPIO10
    - D11 : GPIO11
    - D12 : GPIO12
    - D13 and red LED: GPIO13
    - RGB LED : GPIO17
    - Button BOOT : GPIO7
    - SD-card CARD_DET : GPIO16
    - SD-card SPI0 SD_CLK : GPIO18
    - SD-card SPI0 CMD/MOSI : GPIO19
    - SD-card SPI0 DAT0/MISO : GPIO20
    - SD-card DAT1 : GPIO21
    - SD-card DAT2 : GPIO22
    - SD-card DAT3/CS : GPIO23

See also `pinout`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Adafruit Feather RP2040 Adalogger board does not expose
the SWDIO and SWCLK pins, so programming must be done via the USB
port. Press and hold the BOOT button, and then press the RST button,
and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board
will be reprogrammed.

For more details on programming RP2040-based boards, see
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: adafruit_feather_adalogger_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`input-dump`,
:zephyr:code-sample:`fs`, :zephyr:code-sample:`usb-cdc-acm-console` and
:zephyr:code-sample:`adc_dt` samples.


References
**********

.. target-notes::

.. _Adafruit Feather Adalogger RP2040:
    https://learn.adafruit.com/adafruit-feather-rp2040-adalogger

.. _pinout:
    https://learn.adafruit.com/adafruit-feather-rp2040-adalogger/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-feather-rp2040-pico/downloads
