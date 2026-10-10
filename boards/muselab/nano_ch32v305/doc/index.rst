.. zephyr:board:: nano_ch32v305

Overview
********

The MuseLab nanoCH32V305 is a development board for the RISC-V based CH32V305RBT6
SOC.

The board is equipped with two USB Type-C ports (USB-FS and USB-HS), BOOT and
RST buttons, an FPC-12 LCD connector, an SD card slot and one user LED
connected to PA3.

Hardware
********

The QingKe V4F 32-bit RISC-V processor of the MuseLab nanoCH32V305 is clocked by an
external 8 MHz crystal and runs at 144 MHz. The SOC has 32 KiB of SRAM and 128 KiB
of flash.

The `WCH webpage on CH32V30x`_ contains the processor's information and the datasheet.
MuseLab nanoCH32V305 board schematics can be found on the `nanoCH32V305 git repository`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

The user LED is connected to PA3 and is active low. It is exposed as the ``led0``
alias, so the :zephyr:code-sample:`blinky` sample runs without an overlay.

Serial Port
-----------

``usart1`` is the console and shell UART, wired to PA9 (TX) and PA10 (RX) and
configured for 115200 baud.

.. note::

   The board has no onboard USB-to-UART bridge; both USB Type-C ports are
   wired directly to the SOC's USB controllers. To access the console,
   connect an external 3.3 V USB-to-UART adapter to the PA9/PA10 header pins.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nano_ch32v305`` board can be built and flashed
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
   :board: nano_ch32v305
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
.. _nanoCH32V305 git repository: https://github.com/wuxx/nanoCH32V305
