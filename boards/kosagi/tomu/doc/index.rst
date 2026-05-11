.. zephyr:board:: tomu

Overview
********

`Tomu <https://tomu.im>`_ is a tiny open-hardware board that fits inside a USB
Type-A port. It is based on the Silicon Labs EFM32HG309F64 (Happy Gecko),
an ARM Cortex-M0+ microcontroller running at up to 24 MHz with 64 KB of flash
and 8 KB of SRAM.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Tomu ships with the `Toboot`_ USB DFU bootloader. No external programmer
is needed; flashing is done over USB using ``dfu-util``.

Flashing
========

#. Insert the Tomu into a USB port. The board should enter DFU bootloader mode automatically
   (red and green LEDs blinking interchangeably).

#. Build and flash an application:

   .. zephyr-app-commands::
      :zephyr-app: samples/basic/blinky
      :board: tomu
      :goals: build flash

References
**********

.. target-notes::

.. _Toboot:
   https://github.com/im-tomu/toboot

.. _Tomu hardware:
   https://github.com/im-tomu/tomu-hardware
