.. zephyr:board:: uiapduino_pro_micro_ch32v003


Overview
********

The `UIAPduino Pro Micro CH32V003 V1.4`_ is an open-source hardware development board based on the
WCH `CH32V003`_ microcontroller. The board uses a USB Type-C connector for power and programming,
and can act as a USB 2.0 Low-Speed device for HID applications.

Zephyr support for this board currently provides basic GPIO support and the on-board user LED.


Hardware
********

The board uses the WCH CH32V003F4U6 SoC, which contains a QingKe V2A 32-bit RISC-V core. The SoC
provides 16 KiB of flash and 2 KiB of SRAM, and supports operation up to 48 MHz.

Notable board hardware includes:

* USB Type-C connector for power and programming
* on-board reset button
* on-board power and user LEDs
* selectable 3.3 V or 5 V microcontroller supply
* USB host protection fuses
* through-hole edge connections
* flat bottom for surface-mount-like installation

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 = PC0, active high


Programming
***********

.. zephyr:board-supported-runners::

Applications for the ``uiapduino_pro_micro_ch32v003`` board target can be built and flashed in the
usual way. See :ref:`build_an_application` and :ref:`application_run` for more details.

The board is shipped with a custom USB bootloader based on `rv003usb`_.  To enter writing standby
mode, hold the reset button while connecting the board to USB, then release the reset button.

The Zephyr board configuration uses ``minichlink`` as the default runner and passes ``0x1209b803``
as the device identifier.

Flashing
========

Install ``minichlink`` from `ch32fun`_ and make sure the board can be accessed by the host. On
Linux, this may require installing the corresponding udev rule.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: uiapduino_pro_micro_ch32v003
   :goals: flash


References
**********

.. target-notes::

.. _UIAPduino Pro Micro CH32V003 V1.4:
   https://www.uiap.jp/en/uiapduino/pro-micro/ch32v003/v1dot4
.. _CH32V003:
   https://www.wch-ic.com/products/CH32V003.html
.. _rv003usb:
   https://github.com/cnlohr/rv003usb
.. _ch32fun:
   https://github.com/cnlohr/ch32fun
