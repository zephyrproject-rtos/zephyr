.. zephyr:board:: tiny2040

Overview
********

The `Pimoroni Tiny 2040`_ board is based on the RP2040 microcontroller from Raspberry Pi Ltd.
The board has two 8-pin headers and a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 8 Mbyte QSPI flash
- 8 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- USB type C connector
- Reset and boot buttons
- RGB user LED (red, green and blue controlled individually)


Default Zephyr Peripheral Mapping
=================================

+---------------+--------+------------+
| Description   | Pin    | Comments   |
+===============+========+============+
| Red LED       | GPIO18 | Alias led0 |
+---------------+--------+------------+
| Green LED     | GPIO19 | Alias led1 |
+---------------+--------+------------+
| Blue LED      | GPIO20 | Alias led2 |
+---------------+--------+------------+
| BOOT button   | GPIO23 | Alias sw0  |
+---------------+--------+------------+


GPIO header:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| 0     | GPIO0  | UART0 TX        |
+-------+--------+-----------------+
| 1     | GPIO1  | UART0 RX        |
+-------+--------+-----------------+
| 2     | GPIO2  | I2C1 SDA        |
+-------+--------+-----------------+
| 3     | GPIO3  | I2C1 SCL        |
+-------+--------+-----------------+
| 4     | GPIO4  |                 |
+-------+--------+-----------------+
| 5     | GPIO5  |                 |
+-------+--------+-----------------+
| 6     | GPIO6  |                 |
+-------+--------+-----------------+
| 7     | GPIO7  |                 |
+-------+--------+-----------------+
| A0    | GPIO26 | ADC0            |
+-------+--------+-----------------+
| A1    | GPIO27 | ADC1            |
+-------+--------+-----------------+
| A2    | GPIO28 | ADC2            |
+-------+--------+-----------------+
| A3    | GPIO29 | ADC3            |
+-------+--------+-----------------+

See also `schematic`_.


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
   :board: tiny2040
   :goals: build flash

Note that the red, green and blue parts of the RGB LED are controlled individually by GPIO pins.
By default the red color is blinking when running the sample. The sample does not configure the
GPIO pins for the green and blue parts, which is why these might show a dim light.
See the implementation for the :zephyr:code-sample:`blinky` how the configuration is done,
if you would like to control the other colors.

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`button`,
:zephyr:code-sample:`input-dump` and :zephyr:code-sample:`adc_dt` samples.


References
**********

.. target-notes::

.. _Pimoroni Tiny 2040:
    https://shop.pimoroni.com/products/tiny-2040

.. _schematic:
    https://cdn.shopify.com/s/files/1/0174/1800/files/Tiny2040_PIM558_schematic.pdf
