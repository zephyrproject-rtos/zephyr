.. zephyr:board:: canbed_rp2040

Overview
********

The `Longan Labs CANBed RP2040`_ board is based on the RP2040 microcontroller from Raspberry Pi Ltd.
The board has a CAN bus controller and an I2C connector for easy sensor usage.
It has a micro USB connector.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 13 GPIO pins
- 4 ADC pins
- I2C
- SPI
- UART
- Micro USB connector
- Reset and boot buttons
- Blue user LED
- Grove I2C connector, compatible with Qwiic/Stemma QT if using adapter cable
- Grove UART connector
- CAN bus controller MCP2515
- CAN bus transceiver SN65HVD230


Default Zephyr Peripheral Mapping
=================================

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| LED   | GPIO18 |                 |
+-------+--------+-----------------+


Main header (sorted according to schematic pin numbering):

+-------+--------+-----------------+-------------------+
| Label | Pin    | Default pin mux | Also in connector |
+=======+========+=================+===================+
| GND   |        |                 |                   |
+-------+--------+-----------------+-------------------+
| 24    | GPIO24 |                 |                   |
+-------+--------+-----------------+-------------------+
| 23    | GPIO23 |                 |                   |
+-------+--------+-----------------+-------------------+
| 22    | GPIO22 |                 |                   |
+-------+--------+-----------------+-------------------+
| 21    | GPIO21 |                 |                   |
+-------+--------+-----------------+-------------------+
| SCL   | GPIO7  | I2C1 SCL        | I2C connector     |
+-------+--------+-----------------+-------------------+
| SDA   | GPIO6  | I2C1 SDA        | I2C connector     |
+-------+--------+-----------------+-------------------+
| TX    | GPIO0  | UART0 TX        | UART connector    |
+-------+--------+-----------------+-------------------+
| RX    | GPIO1  | UART0 RX        | UART connector    |
+-------+--------+-----------------+-------------------+
| 10    | GPIO10 |                 |                   |
+-------+--------+-----------------+-------------------+
| 19    | GPIO19 |                 |                   |
+-------+--------+-----------------+-------------------+
| 20    | GPIO20 |                 |                   |
+-------+--------+-----------------+-------------------+
| A3    | GPIO29 | ADC3            |                   |
+-------+--------+-----------------+-------------------+
| 25    | GPIO25 |                 |                   |
+-------+--------+-----------------+-------------------+
| A0    | GPIO26 | ADC0            |                   |
+-------+--------+-----------------+-------------------+
| A1    | GPIO27 | ADC1            |                   |
+-------+--------+-----------------+-------------------+
| A2    | GPIO28 | ADC2            |                   |
+-------+--------+-----------------+-------------------+
| 3V3   |        |                 |                   |
+-------+--------+-----------------+-------------------+


Grove I2C connector (pins also available in the main header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| SCL   | GPIO7  | I2C1 SCL        |
+-------+--------+-----------------+
| SDA   | GPIO6  | I2C1 SDA        |
+-------+--------+-----------------+
| 3V3   |        |                 |
+-------+--------+-----------------+
| GND   |        |                 |
+-------+--------+-----------------+


Grove UART connector (pins also available in the main header):

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| RX    | GPIO1  | UART0 RX        |
+-------+--------+-----------------+
| TX    | GPIO0  | UART0 TX        |
+-------+--------+-----------------+
| 3V3   |        |                 |
+-------+--------+-----------------+
| GND   |        |                 |
+-------+--------+-----------------+


SPI header:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| MISO  | GPIO4  | SPI0 MISO       |
+-------+--------+-----------------+
| SCK   | GPIO2  | SPI0 SCK        |
+-------+--------+-----------------+
| 8     | GPIO8  |                 |
+-------+--------+-----------------+
| GND   |        |                 |
+-------+--------+-----------------+
| MOSI  | GPIO3  | SPI0 MOSI       |
+-------+--------+-----------------+
| 3V3   |        |                 |
+-------+--------+-----------------+


CAN controller:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| CANCS | GPIO9  |                 |
+-------+--------+-----------------+
| INT   | GPIO11 |                 |
+-------+--------+-----------------+

The CAN controller is also connected to the SPI0 pins SCK, MOSI and MISO (see above).

See also `pinout`_.


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
   :board: canbed_rp2040
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world`, :zephyr:code-sample:`adc_dt`
and :zephyr:code-sample:`can-counter` samples.

The use of the Grove/Qwiic/Stemma QT I2C connector is demonstrated using the
:zephyr:code-sample:`light_sensor_polling` sample and a separate shield:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/light_polling
   :board: canbed_rp2040
   :shield: adafruit_veml7700
   :goals: build flash


References
**********

.. target-notes::

.. _Longan Labs CANBed RP2040:
    https://docs.longan-labs.cc/1030018/

.. _pinout:
    https://www.longan-labs.cc/media/wysiwyg/CAN-Bus/CANBed/Details_of_CANBed-04.png
