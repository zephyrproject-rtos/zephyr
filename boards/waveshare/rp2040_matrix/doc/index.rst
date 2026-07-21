.. zephyr:board:: rp2040_matrix

Overview
********

The `Waveshare RP2040-Matrix`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd.
The board has a 5x5 RGB LED matrix and a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 16 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- 5x5 RGB LED matrix (Neopixels)


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - 0 UART0 TX : GPIO0
    - 1 UART0 RX : GPIO1
    - 2 : GPIO2
    - 3 SPI0 MOSI : GPIO3
    - 4 SPI0 MISO : GPIO4
    - 5 : GPIO5
    - 6 SPI0 SCK : GPIO6
    - 7 : GPIO7
    - 8 : GPIO8
    - 9 : GPIO9
    - 10 : GPIO10
    - 11 : GPIO11
    - 12 : GPIO12
    - 13 : GPIO13
    - 14 : GPIO14
    - 15 : GPIO15
    - 26 ADC0 : GPIO26
    - 27 ADC1 : GPIO27
    - 28 ADC2 : GPIO28
    - 29 ADC3 : GPIO29
    - RGB LEDs (Neopixels): GPIO16

See also `schematic`_. The default pin-muxing of UART, SPI and ADC pins is the same as
for the :zephyr:board:`rp2040_zero` board.

.. warning::

    Do not use too many LEDs simultaneously, as the board might get too hot.
    See `Waveshare RP2040-Matrix wiki`_ for details.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Waveshare RP2040-Matrix board does not expose
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

To run the :zephyr:code-sample:`led-strip` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip/
   :board: rp2040_matrix
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`display`,
:zephyr:code-sample:`usb-cdc-acm-console` and :zephyr:code-sample:`adc_dt` samples.


References
**********

.. target-notes::

.. _Waveshare RP2040-Matrix:
    https://www.waveshare.com/rp2040-matrix.htm

.. _Waveshare RP2040-Matrix wiki:
    https://www.waveshare.com/wiki/RP2040-Matrix

.. _schematic:
    https://files.waveshare.com/upload/4/49/RP2040-Matrix.pdf
