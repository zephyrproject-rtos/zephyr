.. zephyr:board:: rp2040_geek

Overview
********

The `Waveshare RP2040-GEEK Development Board`_ is based on the RP2040 microcontroller from
Raspberry Pi Ltd. It has an USB A connector, a color screen, a microSD card slot and several
expansion connectors.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 4 Mbyte QSPI flash
- USB type A connector
- BOOT button
- 135x240 pixels 1.14 inch color LCD screen
- microSD card slot
- 4-pin I2C connector (Quiic / Stemma QT compatible)
- 3-pin UART connector
- 3-pin GPIO connector


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - Connector "UART" pin GP4, UART1 TX : GPIO4
    - Connector "UART" pin GP5, UART1 RX : GPIO5
    - Connector "DEBUG" pin GP2 : GPIO2
    - Connector "DEBUG" pin GP3 : GPIO3
    - Connector "I2C/ADC" pin GP28, I2C0 SDA : GPIO28
    - Connector "I2C/ADC" pin GP29, I2C0 SCL : GPIO29
    - microSD SCK, SPI0 SCK : GPIO18
    - microSD CMD, SPI0 MOSI : GPIO19
    - microSD DO, SPI0 MISO : GPIO20
    - microSD D1 : GPIO21
    - microSD D2 : GPIO22
    - microSD D3 (CS) : GPIO23
    - LCD DC (data/command): GPIO8
    - LCD SPI1 CS : GPIO9
    - LCD SPI1 SCK : GPIO10
    - LCD SPI1 MOSI : GPIO11
    - LCD RST (reset) : GPIO12
    - LCD BL (backlight): GPIO25

This board is intended to be used with a host computer via the USB A connector, why this board
by default uses USB for terminal output.

See also `Waveshare P2040-GEEK wiki`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board does not expose the SWDIO and SWCLK pins, so programming must be done via the
USB port. Press and hold the BOOT button when connecting the RP2040-GEEK to your host computer,
and the device will appear as a USB mass storage unit. Building your application will result in
a :file:`build/zephyr/zephyr.uf2` file. Drag and drop the file to the USB mass storage
unit, and the RP2040-GEEK will be reprogrammed.

For more details on programming RP2040-based boards, see :zephyr:board:`rpi_pico` and especially
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`dining-philosophers` sample to verify output on the USB console:

.. zephyr-app-commands::
   :zephyr-app: samples/philosophers/
   :board: rp2040_geek
   :goals: build flash

Try also the :zephyr:code-sample:`fs`, :zephyr:code-sample:`display`, :zephyr:code-sample:`lvgl`
and :zephyr:code-sample:`uart-passthrough` samples.

Samples where text is printed only just at startup, for example :zephyr:code-sample:`hello_world`,
are difficult to use as the text is already printed once you connect to the newly created
USB console endpoint.

It is easy to connect a sensor shield via the I2C connector, for example
the ``adafruit_lis3dh`` shield. Run the :zephyr:code-sample:`accel_polling` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/accel_polling/
   :board: rp2040_geek
   :shield: adafruit_lis3dh
   :goals: build flash

To use the GPIO pins on the "DEBUG" connector:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: rp2040_geek
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

and then in the device console:

.. code-block:: console

    uart:~$ gpio conf gpio0 2 o
    uart:~$ gpio set gpio0 2 1


References
**********

.. target-notes::

.. _Waveshare RP2040-GEEK Development Board:
    https://www.waveshare.com/rp2040-geek.htm

.. _Waveshare P2040-GEEK wiki:
    https://www.waveshare.com/wiki/RP2040-GEEK

.. _schematic:
    https://files.waveshare.com/wiki/RP2040-GEEK/RP2040-GEEK-Schematic.pdf
