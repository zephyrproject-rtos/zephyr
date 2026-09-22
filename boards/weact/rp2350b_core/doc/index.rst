.. zephyr:board:: rp2350b_core

Overview
********

The WeAct RP2350B Core is a barebone development board for the RP2350B microcontroller by raspberry pi.
It supports running code on either a single Cortex-M33 or a Hazard3 (RISC-V) core.

As with other RP2 boards, there's no support for running any code on the second core.

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 16MB of on-board flash memory
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 48 multi-function GPIO pins including 8 that can be used for ADC
- 2 SPI, 2 I2C, 2 UART, One 12-bit 500ksps Analogue to Digital - Converter (ADC) with 8 channels, 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support

- `rp2350b_core Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- Blue LED on pin 25.
- User button on pin 23.
- It is possible to add a second flash or a PSRAM chip to the back of the board. The CS is Pin 0. This is not currently supported in zephyr.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as for :zephyr:board:`rpi_pico`.
See :ref:`rpi_pico_programming_and_debugging` in :zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky
    :board: rp2350b_core/rp2350b/m33
    :goals: build flash
    :flash-args: -r uf2

.. _rp2350b_core Schematics:
   https://github.com/WeActStudio/WeActStudio.RP2350BCoreBoard/blob/main/HDK/RP2350B_SCH.pdf
