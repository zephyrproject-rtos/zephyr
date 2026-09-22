.. zephyr:board:: rp2350b_plus_w

Overview
********

The Waveshare RP2350B Plus W is a barebone development board built around the Raspberry Pi
RP2350B microcontroller. It supports running code on either a single Cortex-M33 core or a
Hazard3 (RISC-V) core, and includes wireless connectivity through the Raspberry Pi RM2 module
(Wi-Fi and Bluetooth).

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 16MB of on-board flash memory
- Wireless connectivity via the Raspberry Pi RM2 module (2.4 GHz Wi-Fi and Bluetooth 5.2)
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 48 multi-function GPIO pins including 8 that can be used for ADC
- 2 SPI, 2 I2C, 2 UART, One 12-bit 500ksps Analogue to Digital - Converter (ADC) with 8 channels, 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support

- `rp2350b_plus_w Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- Red LED on pin 23.
- It is possible to add a second flash or a PSRAM chip to the back of the board. The CS is Pin 0. This is not currently supported in zephyr.

Wi-Fi Firmware Setup
=====================

Before building applications, you must fetch the required Wi-Fi firmware blobs.

Run the following command to download these blobs:

.. code-block:: console

   west blobs fetch hal_infineon

.. note::

   It is recommended running the command above after :file:`west update`.

This command downloads the necessary firmware files from Infineon's repositories.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as for :zephyr:board:`rpi_pico`.
See :ref:`rpi_pico_programming_and_debugging` in :zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

After fetching the blobs, you can building and flashing the :zephyr:code-sample:`wifi-shell` application.

.. zephyr-app-commands::
    :zephyr-app: samples/net/wifi/shell
    :board: rp2350b_plus_w/rp2350b/m33
    :goals: build flash
    :flash-args: -r uf2

.. _rp2350b_plus_w Schematics:
   https://files.waveshare.com/wiki/RP2350B-Plus-W/RP2350B-Plus-W.pdf
