.. zephyr:board:: 32f967_dv

Elan 32f967_dv
##############

Overview
********

The Elan 32f967_dv is a B2B development board based on the Elan em32f967
SoC (ARM Cortex-M4). This board is used to validate the initial SoC
integration with Zephyr.

Hardware
********

The platform provides the following hardware components:

- SoC: Elan em32f967 (ARM Cortex-M4)
- Maximum CPU frequency: 96 MHz
- Embedded Flash: 536 KB
- Embedded RAM: 272 KB
- UART (debug via soldered jump wires, no dedicated connector)
- SPI
- GPIO
- PWM
- USB (used for firmware flashing and application communication)
- Watchdog Timer (WDT)
- Backup domain registers (accessed via Zephyr BBRAM interface)
- True Random Number Generator (TRNG)
- Hardware Crypto Engine
- Timer
- Real-Time Clock (RTC)

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- **UART_1 TX/RX** : PA2 / PA1
- **SPI_2 NSS/SCK/MISO/MOSI** : PB4 / PB5 / PB6 / PB7
- **PWM_0 LED** : PA3

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

This board does not use a standard flashing interface such as J-Link or OpenOCD.

Flashing
========

The flashing tool is distributed only to Elan's customers for production and
evaluation purposes. It is not publicly available.

At this stage, the Zephyr ``west flash`` command is not supported.

You can build applications in the usual way. Here is an example for
the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: 32f967_dv
   :goals: build

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: 32f967_dv
   :goals: debug

References
**********

Documentation for this board and the Elan em32f967 SoC is available to Elan customers.
Please contact Elan for datasheets, technical reference manuals, and tooling information.
