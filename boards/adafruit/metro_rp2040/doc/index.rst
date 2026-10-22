.. zephyr:board:: adafruit_metro_rp2040

Overview
********

The `Adafruit Metro RP2040`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd. The board has a MicroSD-card socket
for storing data, and a Stemma QT connector for easy sensor usage.
It is compatible with the Arduino board form factor, and has
a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 16 Mbyte QSPI flash
- 20 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Barrel power supply connector for 6-12 Volt DC
- On/off switch
- Reset and boot buttons
- Red LED
- RGB LED (Neopixel)
- Stemma QT I2C connector
- MicroSD-card holder
- Debug connectors (3-pin and 10-pin)


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - A0 ADC0 : GPIO26
    - A1 ADC1 : GPIO27
    - A2 ADC2 : GPIO28
    - A3 ADC3 : GPIO29
    - D24 : GPIO24
    - D25 : GPIO25
    - RX UART0: GPIO1
    - TX UART0: GPIO0
    - D2 : GPIO2
    - D3 : GPIO3
    - D4 : GPIO4
    - D5 : GPIO5
    - D6 : GPIO6
    - D7 : GPIO7
    - D8 : GPIO8
    - D9 : GPIO9
    - D10 : GPIO10
    - D11 : GPIO11
    - D12 : GPIO12
    - D13 Red LED : GPIO13
    - SDA I2C0: GPIO16
    - SCL I2C0: GPIO17
    - RGB LED (Neopixel) : GPIO14
    - SD-card CARD_DET : GPIO15
    - SD-card SPI0 SCK/SDIO_CLK : GPIO18
    - SD-card SPI0 MOSI/SDIO_CMD : GPIO19
    - SD-card SPI0 MISO/SDIO_DAT0 : GPIO20
    - SD-card SPI0 SDCS/SDIO_DAT3 : GPIO23
    - SD-card SDIO_DAT1 : GPIO21
    - SD-card SDIO_DAT2 : GPIO22

The SD-card SPI signals (MOSI, MISO and SCK) are available on the 6-pin male header.

There is a switch for swapping the RX and TX position on the Arduino header.
The devicetree file is using the switch position RX=1 TX=0, as the RP2040 UART0 TX
is connected to GPIO0.

Normally an Arduino header has SPI functionality on the pins D10, D11, D12 and D13.
However the SPI chip select and SPI clock pin positions are swapped on the Adafruit
Metro RP2040 board.

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

It is also possible to program and debug the board via the 3-pin or 10-pin
debug connectors. Then a separate programming hardware tool is required, and
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
   :board: adafruit_metro_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`led-strip`,
:zephyr:code-sample:`fs`, :zephyr:code-sample:`usb-cdc-acm-console` and
:zephyr:code-sample:`adc_dt` samples.

The Stemma QT connector can be used to read sensor data via I2C, for
example the :zephyr:code-sample:`accel_polling` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling
   :board: adafruit_metro_rp2040
   :shield: adafruit_lis3dh
   :goals: build flash


References
**********

.. target-notes::

.. _Adafruit Metro RP2040:
    https://learn.adafruit.com/adafruit-metro-rp2040

.. _pinout:
    https://learn.adafruit.com/adafruit-metro-rp2040/pinouts

.. _schematic:
    https://learn.adafruit.com/adafruit-metro-rp2040/downloads
