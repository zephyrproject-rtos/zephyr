.. zephyr:board:: beetle_rp2350

Overview
********

The `DFRobot Beetle RP2350`_ board is based on the RP2350A microcontroller from Raspberry Pi Ltd.
The board has two 8-pin headers and a USB type C connector.


Hardware
********

- Microcontroller Raspberry Pi RP2350A, with a max frequency of 150 MHz
- Dual ARM Cortex M33 cores, and dual RISC-V Hazard3 cores.
- 520 kByte SRAM
- 2 Mbyte QSPI flash
- 9 GPIO pins
- 2 ADC pins
- I2C
- UART
- SPI
- USB type C connector
- Lithium battery charger
- Reset and boot buttons
- User LED


Default Zephyr Peripheral Mapping
=================================

+---------------+--------+------------+
| Description   | Pin    | Comments   |
+===============+========+============+
| User LED      | GPIO25 | Alias led0 |
+---------------+--------+------------+


GPIO header:

+-------+--------+-----------------+
| Label | Pin    | Default pin mux |
+=======+========+=================+
| 0     | GPIO0  | UART0 TX        |
+-------+--------+-----------------+
| 1     | GPIO1  | UART0 RX        |
+-------+--------+-----------------+
| 4     | GPIO4  | I2C0 SDA        |
+-------+--------+-----------------+
| 5     | GPIO5  | I2C0 SCL        |
+-------+--------+-----------------+
| 8     | GPIO8  |                 |
+-------+--------+-----------------+
| 9     | GPIO9  |                 |
+-------+--------+-----------------+
| 16    | GPIO16 | SPI0 MISO       |
+-------+--------+-----------------+
| 18    | GPIO18 | SPI0 SCK        |
+-------+--------+-----------------+
| 19    | GPIO19 | SPI0 MOSI       |
+-------+--------+-----------------+
| 26    | GPIO26 | ADC0            |
+-------+--------+-----------------+
| 27    | GPIO27 | ADC1            |
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

It is also possible to program and debug the board via the SWDIO and SWCLK pins in the DEBUG
connector. You must solder a 3-pin or 4-pin header to the back of the board in order to use
this feature. A separate programming hardware tool is required, and for example
the :command:`openocd` software is used. You might need to use Raspberry Pi's forked
version of OpenOCD. Typically the ``OPENOCD`` and ``OPENOCD_DEFAULT_PATH`` values should be
set when building, and the ``--runner openocd`` argument should be used when flashing.
For more details on programming RP2040-based and RP2350-based boards,
see :ref:`rpi_pico_programming_and_debugging`.


Flashing the M33 core
=====================

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: beetle_rp2350/rp2350a/m33
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world` and
:zephyr:code-sample:`adc_dt` samples.

Use the shell to control the GPIO pins:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/sensor_shell
   :board: beetle_rp2350/rp2350a/m33
   :gen-args: -DCONFIG_GPIO=y -DCONFIG_GPIO_SHELL=y
   :goals: build flash

To set one of the GPIO pins high, use these commands in the shell:

.. code-block:: shell

    gpio conf gpio0 8 o
    gpio set gpio0 8 1


Flashing the Hazard3 core
=========================

The RP2350A microcontroller has two ARM M33 cores and two RISC-V Hazard3 cores.
To flash one of the Hazard3 cores, use the board argument ``beetle_rp2350/rp2350a/hazard3``.
The sample :zephyr:code-sample:`blinky` has been verified for this core.
Use the USB mass storage programming method described above.


References
**********

.. target-notes::

.. _DFRobot Beetle RP2350:
    https://www.dfrobot.com/product-2913.html

.. _pinout:
    https://wiki.dfrobot.com/SKU_DFR1188_Beetle_RP2350#target_4

.. _schematic:
    https://dfimg.dfrobot.com/5d57611a3416442fa39bffca/wiki/f18e5f3a683e6d8a9c8582ac6f89b023.pdf
