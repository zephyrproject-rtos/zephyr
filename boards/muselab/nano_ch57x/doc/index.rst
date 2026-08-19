.. zephyr:board:: nano_ch57x

Overview
********

The MuseLab `nanoCH57x`_ is an evaluation board for the RISC-V based CH570/CH572
SoC.

The board is equipped with a power LED, reset button, USB port, and one user
LED. It also features 2.4G PCB antenna and supports BLE/2.4GHz RF transceiver.

Hardware
********

The `QingKe V3C 32-bit RISC-V processor`_ of the nanoCH57x runs at up to
100 MHz, using a 32 MHz external crystal.

The `WCH webpage on CH572`_ contains the processor's information and the
datasheet. The ``nanoCH57x`` board schematics can be found on
`nanoCH57x git repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the CH570/CH572 SoC can be routed to various pins on the
board. These mappings can be configured through DTS. The following table
shows the possible routings for each peripheral.

+------------------+-------------------------------+---------------------------+
| Peripheral       | Possible Pin(s)               | Default Pin               |
+==================+===============================+===========================+
| UART0 RXD        | PA2, PA3, PA0, PA1, PA6, PA9, | PA2                       |
|                  | PA10, PA11                    |                           |
+------------------+-------------------------------+---------------------------+
| UART0 TXD        | PA3, PA2, PA1, PA0, PA7, PA8, | PA3                       |
|                  | PA11, PA10                    |                           |
+------------------+-------------------------------+---------------------------+
| IC20 SCL         | PA8, PA0, PA3, PA5            | PA8                       |
+------------------+-------------------------------+---------------------------+
| IC20 SDA         | PA9, PA1, PA2, PA6            | PA9                       |
+------------------+-------------------------------+---------------------------+
| SPI0 MOSI        | PA7                           | PA7                       |
+------------------+-------------------------------+---------------------------+
| SPI0 MISO        | PA6                           | PA6                       |
+------------------+-------------------------------+---------------------------+
| SPI0 SCK         | PA5, PA3                      | PA5                       |
+------------------+-------------------------------+---------------------------+
| SPI0 SCS         | PA4, PA2                      | PA4                       |
+------------------+-------------------------------+---------------------------+
| TMR0 - PWM0      | PA7, PA2, PA4, PA9            | PA7                       |
+------------------+-------------------------------+---------------------------+
| TMR0 - CAPIN1    | PA7, PA2, PA4, PA9            | PA7                       |
+------------------+-------------------------------+---------------------------+
| TMR0 - CAPIN2    | PA2, PA7, PA9, PA4            | PA2                       |
+------------------+-------------------------------+---------------------------+
| PWM (1-5)        | PA7, PA2, PA3, PA4, PA8       | PA7, PA2, PA3, PA4, PA8   |
+------------------+-------------------------------+---------------------------+
| KEYSCAN (0-4)    | PA2, PA3, PA8, PA10, PA11     | PA2, PA3, PA8, PA10, PA11 |
+------------------+-------------------------------+---------------------------+
| CMP0 - N         | PA2                           | PA2                       |
+------------------+-------------------------------+---------------------------+
| CMP0 - P         | PA3, PA7                      | PA3                       |
+------------------+-------------------------------+---------------------------+

Programming and Debugging
*************************

Applications for the ``nanoCH57x`` board target can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details); however, an external programmer (like the `WCH LinkE`_) is
recommended to flash and debug the board easily.

In this case, the pins of the external programmer must be connected as follows:

+-----------+-----+-----+-------+-------+------+------+
| WCH LinkE | 3V3 | GND | SWDIO | SWCLK | TX   | RX   |
+-----------+-----+-----+-------+-------+------+------+
| nanoCH57x | 3V3 | G   | PA0   | PA1   | PA2  | PA3  |
+-----------+-----+-----+-------+-------+------+------+

.. NOTE:: The debug interface must be enabled in the board and the code
   protection must be disabled.

.. zephyr:board-supported-runners::

To use OpenOCD runner, you need to use an WCH OpenOCD. E.g. the WCH OpenOCD
liberated fork, available at https://github.com/jnk0le/openocd-wch.

.. NOTE:: You can change the default OpenOCD binary used by Zephyr with the
   following methods:

   1. Specify the path each time you flash/debug the board by adding the
      ``--openocd path/to/wch-openocd/bin/openocd`` argument to the ``west flash`` or
      ``west debug`` command.

   2. Set the WCH OpenOCD binary as a build configuration parameter:

      .. code-block:: console

         west config build.cmake-args -- "-DOPENOCD=path/to/wch-openocd/bin/openocd"

      This method only needs to be run once, unlike the first one.

Flashing
========

Here is an example using the :zephyr:code-sample:`blinky` application.
The LED definitions can be found in
:zephyr_file:`boards/muselab/nano_ch57x/nano_ch57x_common.dtsi`.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nano_ch57x/ch570
   :goals: build flash

Debugging
=========

This board can be debugged via OpenOCD:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nano_ch57x/ch570
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _nanoCH57x: https://github.com/wuxx/nanoCH57x
.. _QingKe V3C 32-bit RISC-V processor: https://www.wch-ic.com/downloads/QingKeV3_Processor_Manual_PDF.html
.. _WCH webpage on CH572: https://www.wch-ic.com/products/CH572.html
.. _nanoCH57x git repository: https://github.com/wuxx/nanoCH57x/blob/main/hardware/nanoCH57x-v1.0.pdf
.. _WCH LinkE: https://www.wch-ic.com/products/WCH-Link.html
