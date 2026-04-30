.. zephyr:board:: nano_ch32v317

Overview
********

The MuseLab nanoCH32V317 is an development board for the RISC-V based CH32V317WCU6
SOC.

The board is equipped with a reset button, a boot button, one user LED, two USB
ports and an Ethernet Jack.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the WCH nanoCH32V317 is clocked by an
external crystal and runs at 144 MHz.

The chip's most unique feature is an integrated Ethernet MAC and PHY that supports
100BASE-TX Ethernet.

The `WCH webpage on CH32V30x`_ contains the processor's information and the datasheet.
WCH nanoCH32V317 board schematics can be found on `nanoCH32V317 git repository`_.

.. warning::

   There is no USB VBUS diode on the board so be careful not to power a device
   from USB and external supply simultaneously!

Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nano_ch32v317`` board can be built and flashed
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
   :board: nano_ch32v317
   :goals: build flash


Or you can use :zephyr:code-sample:`dhcpv4-client` sample to test evaluate the built-in
Ethernet peripheral

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcpv4_client
   :board: nano_ch32v317
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
.. _nanoCH32V317 git repository: https://github.com/wuxx/nanoCH32V317
