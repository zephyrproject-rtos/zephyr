.. zephyr:board:: ch32v307v_evt_r1

Overview
********

The `WCH`_ CH32V307V-EVT-R1 is an evaluation board for the RISC-V based CH32V307VCT6
SOC.

The board is equipped with a power LED, reset button, USB port for power, and
two user LEDs. It also features Ethernet port and built-in programmer.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the WCH CH32V307V-EVT-R1 is clocked by an
external crystal and runs at 144 MHz.

The `WCH webpage on CH32V30x`_ contains the processor's information and the datasheet.
WCH CH32V307V-EVT-R1 board schematics can be found on `CH32V307 openwch git repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

Board LEDs are not connected to SoC in layout. For ``blinky`` and ``threads`` sample
applications you have to use jumper wires to connect them to I/O pins. You also need
to change leds status to "okay" in :zephyr_file:`boards/wch/ch32v307v_evt_r1/ch32v307v_evt_r1.dts`.

.. list-table:: LED connection
   :header-rows: 1

   * - J3 header
     - J4 header
   * - LED1
     - PD0
   * - LED2
     - PD1


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ch32v307v_evt_r1`` board can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details). The board includes `WCH LinkE`_ programmer accessible on
USB-C port P9.

Flashing
========

You can use minichlink_ to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch32v307v-evt-r1
   :goals: build flash


Or you can use :zephyr:code-sample:`multi-thread-blinky` sample to test both LEDs.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/threads
   :board: ch32v307v-evt-r1
   :goals: build flash


Debugging
=========

This board can be debugged via ``minichlink``.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V30x: https://www.wch-ic.com/downloads/CH32V20x_30xDS0_PDF.html
.. _minichlink: https://github.com/cnlohr/ch32fun/tree/master/minichlink
.. _WCH LinkE: https://www.wch-ic.com/downloads/WCH-LinkUserManual_PDF.html
.. _CH32V307 openwch git repository: https://github.com/openwch/ch32v307/blob/main/SCHPCB/CH32V307V-R1-1v0/SCH_PCB/CH32V307V-R1.pdf
