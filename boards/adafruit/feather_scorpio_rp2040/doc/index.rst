.. zephyr:board:: adafruit_feather_scorpio_rp2040

Overview
********

The `Adafruit RP2040 Scorpio Feather`_ board is based on the RP2040
microcontroller from Raspberry Pi Ltd.
It has 8 outputs for external RGB LEDs (Neopixels), and a Stemma QT connector for easy
sensor usage. It is compatible with the Feather board form factor, and has a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 25 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- On-board Red LED
- On-board RGB LED (Neopixel)
- Stemma QT I2C connector (also known as Qwiic)
- Built-in lithium ion battery charger
- 8 external neopixel outputs


Default Zephyr Peripheral Mapping
=================================

+------------------+--------+-----------------+-------------------+
| Description      | Pin    | Default pin mux | Comment           |
+==================+========+=================+===================+
| Boot button      | GPIO7  |                 | Alias sw0         |
+------------------+--------+-----------------+-------------------+


Feather header:

+-------+--------+-----------------+-------------------+
| Label | Pin    | Default pin mux | Also used in      |
+=======+========+=================+===================+
| A0    | GPIO26 | ADC0            |                   |
+-------+--------+-----------------+-------------------+
| A1    | GPIO27 | ADC1            |                   |
+-------+--------+-----------------+-------------------+
| A2    | GPIO28 | ADC2            |                   |
+-------+--------+-----------------+-------------------+
| A3    | GPIO29 | ADC3            |                   |
+-------+--------+-----------------+-------------------+
| 24    | GPIO24 |                 |                   |
+-------+--------+-----------------+-------------------+
| 25    | GPIO25 |                 |                   |
+-------+--------+-----------------+-------------------+
| SCK   | GPIO14 | SPI1 SCK        |                   |
+-------+--------+-----------------+-------------------+
| MO    | GPIO15 | SPI1 MOSI       |                   |
+-------+--------+-----------------+-------------------+
| MI    | GPIO8  | SPI1 MISO       |                   |
+-------+--------+-----------------+-------------------+
| R     | GPIO1  | UART0 RX        |                   |
+-------+--------+-----------------+-------------------+
| T     | GPIO0  | UART0 TX        |                   |
+-------+--------+-----------------+-------------------+
| 4     | GPIO4  | PIO0            | On-board Neopixel |
+-------+--------+-----------------+-------------------+
| D     | GPIO2  | I2C1 SDA        | Stemma QT         |
+-------+--------+-----------------+-------------------+
| C     | GPIO3  | I2C1 SCL        | Stemma QT         |
+-------+--------+-----------------+-------------------+
| 5     | GPIO5  |                 |                   |
+-------+--------+-----------------+-------------------+
| 6     | GPIO6  |                 |                   |
+-------+--------+-----------------+-------------------+
| 9     | GPIO9  |                 |                   |
+-------+--------+-----------------+-------------------+
| 10    | GPIO10 |                 |                   |
+-------+--------+-----------------+-------------------+
| 11    | GPIO11 |                 |                   |
+-------+--------+-----------------+-------------------+
| 12    | GPIO12 |                 |                   |
+-------+--------+-----------------+-------------------+
| 13    | GPIO13 |                 | Red user LED      |
+-------+--------+-----------------+-------------------+


Stemma QT I2C connector (pins also available in the Feather header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| SDA   | GPIO2  | I2C1 SDA        |
+-------+--------+-----------------+
| SCL   | GPIO3  | I2C1 SCL        |
+-------+--------+-----------------+


External Neopixel header:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| 0     | GPIO16 | PIO1            |
+-------+--------+-----------------+
| 1     | GPIO17 |                 |
+-------+--------+-----------------+
| 2     | GPIO18 |                 |
+-------+--------+-----------------+
| 3     | GPIO19 |                 |
+-------+--------+-----------------+
| 4     | GPIO20 |                 |
+-------+--------+-----------------+
| 5     | GPIO21 |                 |
+-------+--------+-----------------+
| 6     | GPIO22 |                 |
+-------+--------+-----------------+
| 7     | GPIO23 |                 |
+-------+--------+-----------------+

See also `pinout`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

By default programming is done via the USB connector. Press and hold the BOOT button, and then
press the RST button, and the device will appear as a USB mass storage unit.
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

It is also possible to program and debug the board via the SWDIO and SWCLK pins.
Then a separate programming hardware tool is required, and for example the :command:`openocd`
software is used. Typically the ``OPENOCD`` and ``OPENOCD_DEFAULT_PATH`` values should be set
when building, and the ``--runner openocd`` argument should be used when flashing.
For more details on programming RP2040-based boards, see :ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky/
    :board: adafruit_feather_scorpio_rp2040
    :goals: build flash

Try also the :zephyr:code-sample:`led-strip`, :zephyr:code-sample:`input-dump`,
:zephyr:code-sample:`hello_world` and :zephyr:code-sample:`adc_dt` samples.

The :zephyr:code-sample:`button` sample might show spurious button presses, as the button is
connected to a line also used for the QSPI memory.

The use of the Stemma QT I2C connector is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
    :zephyr-app: samples/sensor/light_polling/
    :board: adafruit_feather_scorpio_rp2040
    :shield: adafruit_veml7700
    :goals: build flash

As example usage of external Neopixels via the board edge header, the devicetree describes a
string of Neopixels connected to output 0. You need to adapt the number of pixels given in
the ``external_ws2812`` node to the actual number of pixels in your external light-strip.
You must also change the ``aliases`` attribute from ``led-strip = &onboard_ws2812;`` to
``led-strip = &external_ws2812;``. Then use the :zephyr:code-sample:`led-strip` sample.

It is possible to enable Neopixel drivers for the other external output pins, by adjusting the
devicetree file in a similar manner.


References
**********

.. target-notes::

.. _Adafruit RP2040 Scorpio Feather:
    https://learn.adafruit.com/introducing-feather-rp2040-scorpio

.. _pinout:
    https://learn.adafruit.com/introducing-feather-rp2040-scorpio/pinouts

.. _schematic:
    https://learn.adafruit.com/introducing-feather-rp2040-scorpio/downloads
