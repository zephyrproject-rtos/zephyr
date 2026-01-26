.. zephyr:board:: w6300_evb_pico2

Overview
********

W6300-EVB-Pico2 is a microcontroller evaluation board based on the Raspberry
Pi RP2350A and the WIZnet W6300 hardwired TCP/IP controller. It works like the
Raspberry Pi Pico2, with additional Ethernet connectivity provided by the
W6300. The USB bootloader allows flashing without an adapter. SWD is supported
for debugging with external adapters.

Hardware
********

- Dual core Arm Cortex-M33 processor running up to 150 MHz
- 520 KB on-chip SRAM
- 16 MB on-board QSPI flash with XIP capabilities
- 26 GPIO pins
- 3 analog inputs
- 2 UART peripherals
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 3 programmable I/O (PIO) blocks
- On-board LED
- 1 watchdog timer peripheral
- WIZnet W6300 Ethernet MAC/PHY with IPv4/IPv6 support

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

The peripherals of the RP2350A SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

The W6300 is connected using SPI/QSPI signals on GPIO15-22:

+--------+-------------+------------------+
| GPIO   | W6300 Pin   | Function         |
+========+=============+==================+
| GPIO15 | INTn        | Interrupt        |
+--------+-------------+------------------+
| GPIO16 | CSn         | Chip Select      |
+--------+-------------+------------------+
| GPIO17 | SCLK        | SPI Clock        |
+--------+-------------+------------------+
| GPIO18 | IO0         | MOSI / QSPI IO0   |
+--------+-------------+------------------+
| GPIO19 | IO1         | MISO / QSPI IO1   |
+--------+-------------+------------------+
| GPIO20 | IO2         | QSPI IO2          |
+--------+-------------+------------------+
| GPIO21 | IO3         | QSPI IO3          |
+--------+-------------+------------------+
| GPIO22 | RSTn        | Reset             |
+--------+-------------+------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as or
:zephyr:board:`rpi_pico`. See :ref:`rpi_pico_programming_and_debugging` in
:zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using
Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: w6300_evb_pico2/rp2350a/m33
   :goals: build flash
   :flash-args: --openocd /usr/local/bin/openocd

.. target-notes::

.. _pico_setup.sh:
  https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Raspberry Pi Pico:
  https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _W6300 Evaluation Board Pico2 Documentation:
  https://docs.wiznet.io/Product/Chip/Ethernet/W6300/W6300-EVB-Pico2
