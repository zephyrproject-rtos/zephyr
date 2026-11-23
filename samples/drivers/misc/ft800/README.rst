.. zephyr:code-sample:: ft800
   :name: FT800
   :relevant-api: ft8xx_interface

   Display various shapes and text using FT800 Embedded Video Engine.

Overview
********

This sample displays a hello message and an incrementing counter using FT800
Embedded Video Engine.

Requirements
************

To use this sample, the following hardware is required:

* A board with SPI support
* Display with `FT800 EVE`_ like `VM800C board`_

Wiring
******

You will need to connect the `FT800 EVE`_ SPI interface onto a board that
supports Arduino shields.

Building and Running
********************

This sample should work on any board that has SPI enabled and has an Arduino
shield interface. For example, it can be run on the nRF52840-DK board as
described below:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/ft800
   :board: nrf52840dk/nrf52840
   :goals: flash
   :compact:

To build the sample for `VM800C board`_ the shield must be defined as described
below:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/ft800
   :board: nrf52840dk/nrf52840
   :shield: ftdi_vm800c
   :goals: flash
   :compact:

.. _VM800C board: https://www.ftdichip.com/old2020/Products/Modules/VM800C.html
.. _FT800 EVE: https://www.ftdichip.com/old2020/Products/ICs/FT800.html
