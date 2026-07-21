.. zephyr:board:: adafruit_itsybitsy_rp2040

Overview
********

The `Adafruit Itsybitsy RP2040`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd.
It is compatible with the Itsybitsy board form factor, and has
a USB micro B connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 12 GPIO pins
- 4 ADC pins
- SWDIO and SWCLK pins for programming and debugging
- I2C
- SPI
- UART
- USB micro B connector
- Reset and boot buttons
- Red LED
- RGB LED (Neopixel)


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - A0 ADC0 : GPIO26
    - A1 ADC1 : GPIO27
    - A2 ADC2 : GPIO28
    - A3 ADC3 : GPIO29
    - D24: GPIO24
    - D25: GPIO25
    - SCK SPI0 SCK : GPIO18
    - MO SPI0 MOSI : GPIO19
    - MI SPI0 MISO : GPIO20
    - D2 : GPIO12
    - ENABLE : -
    - SWDIO : -
    - SWCLK : -
    - D3 : GPIO5
    - D4 : GPIO4
    - RX UART0: GPIO1
    - TX UART0: GPIO0
    - SDA I2C1 : GPIO2
    - SCL I2C1 : GPIO3
    - D5 (5 Volt digital out): GPIO14
    - D7 : GPIO6
    - D9 : GPIO7
    - D10 : GPIO8
    - D11 : GPIO9
    - D12 : GPIO10
    - D13 and red LED : GPIO11
    - Button USBBOOT : GPIO13
    - RGB LED (Neopixel) signal : GPIO17
    - RGB LED (Neopixel) power : GPIO16

See also `pinout`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

By default programming is done via the USB connector.
Press and hold the BOOT button, and then press the RST button,
and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board
will be reprogrammed.

It is also possible to program and debug the board via the SWDIO and SWCLK pins.
Then a separate programming hardware tool is required, and
for example the :command:`openocd` software is used. Typically the
``OPENOCD`` and ``OPENOCD_DEFAULT_PATH``
values should be set when building, and the ``--runner openocd``
argument should be used when flashing.
For more details on programming RP2040-based boards, see
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: adafruit_itsybitsy_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`led-strip`,
:zephyr:code-sample:`input-dump`, :zephyr:code-sample:`usb-cdc-acm-console` and
:zephyr:code-sample:`adc_dt` samples.


References
**********

.. target-notes::

.. _Adafruit Itsybitsy RP2040:
    https://learn.adafruit.com/adafruit-itsybitsy-rp2040

.. _pinout:
    https://learn.adafruit.com/adafruit-itsybitsy-rp2040/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-itsybitsy-rp2040/downloads
