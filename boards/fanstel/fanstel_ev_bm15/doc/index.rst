.. zephyr:board:: fanstel_ev_bm15

Overview
********

The Fanstel EV-BM15 is an evaluation board based on the Nordic nRF54L15 SoC.
It is designed for development and evaluation of Bluetooth Low Energy (BLE)
and other wireless applications.

Hardware
********

- Nordic nRF54L15 SoC
- On-board LEDs and buttons
- UART interface
- GPIO headers for expansion

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board provides the following peripherals:

- 2 LEDs
- 2 Buttons
- UART interfaces
- GPIOs for general-purpose use

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

To build an application for the Fanstel EV-BM15 board, use:

.. code-block:: console

   west build -b fanstel_ev_bm15/nrf54l15/cpuapp samples/hello_world

Flashing
========

To flash the application:

.. code-block:: console

   west flash

Debugging
=========

Debugging is supported via standard Zephyr debug tools such as J-Link.

References
**********
- Nordic nRF54L15 Product Specification
- Fanstel official website: https://www.fanstel.com
