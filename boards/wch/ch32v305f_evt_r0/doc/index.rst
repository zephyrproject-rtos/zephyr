.. zephyr:board:: ch32v305f_evt_r0

Overview
********

The `WCH`_ CH32V305F-EVT-R0 is an evaluation board for the RISC-V based CH32V305FBP6
SOC.

The board is equipped with a power LED, reset button, USB port for power/data, and
two user LEDs.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the WCH CH32V307V-EVT-R1 is clocked by an
external crystal and runs at 144 MHz.

The `WCH webpage on CH32V30x`_ contains the processor's information and the datasheet.
WCH CH32V305F-EVT-R10 board schematics can be found on `CH32V307 openwch git repository`_.
Note that the documentation for the CH32V305 is located under the CH32V307 pages.

.. warning::

   Most WCH dev kits do not come with the USB CC line pull-downs fitted. To power the
   board from a USB C adapter you need to fit them. However there is no USB VBUS diode
   on the board so be careful not to power a device from USB and external supply
   simultaneously!

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

Board LEDs are not connected to SoC in layout. For ``blinky`` and ``threads`` sample
applications you have to use jumper wires to connect them to I/O pins. You also need
to change leds status to "okay" in :zephyr_file:`boards/wch/ch32v305f_evt_r0/ch32v305f_evt_r0_common.dtsi`.

.. list-table:: LED connection
   :header-rows: 1

   * - P4 header
     - P2 header
   * - LED1
     - PB6
   * - LED2
     - PB7


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ch32v305f_evt_r0`` board can be built and flashed
in the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

You can use minichlink_ to flash the board. Once ``minichlink`` has been set
up, build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details). wlink_ is an alternative tool for using
the WCH programmers. Alternatively, wchisp_ can be used to flash via the USB bootloader.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch32v305f_evt_r0
   :goals: build flash


Or you can use :zephyr:code-sample:`multi-thread-blinky` sample to test both LEDs.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/threads
   :board: ch32v305f_evt_r0
   :goals: build flash


Debugging
=========

This board can be debugged via ``minichlink`` / ``openocd``.

References
**********

.. target-notes::

.. _WCH: http://www.wch-ic.com
.. _WCH webpage on CH32V30x: https://www.wch-ic.com/downloads/CH32V20x_30xDS0_PDF.html
.. _minichlink: https://github.com/cnlohr/ch32fun/tree/master/minichlink
.. _wlink: https://github.com/ch32-rs/wlink
.. _wchisp: https://github.com/ch32-rs/wchisp
.. _CH32V307 openwch git repository: https://github.com/openwch/ch32v307/blob/main/SCHPCB/CH32V307V-R1-1v0/SCH_PCB/CH32V307V-R1.pdf
