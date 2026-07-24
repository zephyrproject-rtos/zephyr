.. zephyr:board:: nrf52840_stamp_xl

Overview
********

The nRF52840 is a limited-edition nRF52840 development board designed
by Icy Electronics for `Hackclub`. It features a nRF52840 on a small
stamp-like form.

It has castellated edges on 3 of the sides, allowing it to be easily
soldered onto a carrier board, without having to deal with the aQFN
nRF52840 footprint. The board breaks out 36 I/O pins, and also has an
embedded 24 pin e-ink connector.

More details are available in the `nRF52840 Stamp`_ GitHub repo

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LED
---

* LED0 = P0.18
* LED1 = P0.17

E-Ink
---

* BUSY   = P0.26
* nRESET = P0.04
* nD/C   = P0.06
* nCS    = P0.08
* CLK    = P1.09
* MOSI   = P0.11

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nrf52840_stamp_xl/nrf52840`` board target can be
built in the usual way (see :ref:`build_an_application` for more details).

Flashing
========

The board is factory-programmed with Adafruit's UF2 bootloader

#. Reset the board into the bootloader by bridging ground and RST 2 times
quickly

   The status LED should start a fade pattern, signalling the bootloader is
   running.

#. Compile a Zephyr application; we'll use :zephyr:code-sample:`blinky`.

   .. zephyr-app-commands::
      :app: zephyr/samples/basic/blinky
      :board: nrf52840_stamp_xl/nrf52840/uf2
      :goals: build

#. Flash it onto the board. You may need to mount the device.

   .. code-block:: console

      west flash

   When this command exits, observe the red LED on the board blinking,


Debugging
=========

You may debug this board using the broken out pads on the top.
PyOCD and openOCD can be used to flash and debug this board.

References
**********

.. target-notes::

.. _Hackclub:
   https://hackclub.com/
.. _nRF52840 Stamp:
   https://github.com/cheyao/nrf52840-stamp-xl
