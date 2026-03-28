.. zephyr:board:: frdm_mcxl255

Overview
********

FRDM-MCXL255 is compact and scalable development board for rapid prototyping of
MCX L25x MCUs. It offers industry standard headers for easy access to the
MCUs I/Os, integrated open-standard serial interfaces, external flash memory and
an on-board MCU-Link debugger. MCX L Series features Adaptive Dynamic Voltage
Control (ADVC) for optimized power consumption at low frequency operation.
Compared to traditional low-power MCUs, a dedicated ultra-low-power (ULP)
Sense Domain allows operation of low-power peripherals while keeping
the main core in Deep Power Mode. This avoids event triggering and keeps data
acquisition to extremely low power levels.

Hardware
********
- NXP MCX L255 MCU
- On-board MCU-Link debugger with CMSIS-DAP
- Arduino Header, mikroBUS, Pmod, Sensor port

Main domain
===========

- 96MHz Cortex-M33 (with FPU, MPU, SIMD, Trustzone with 8 SAU regions)
- 512KB on chip Flash dual-bank on chip Flash
- 128KB RAM
- Timers: 3x 32b Timers, Micro Tick Timer, Windowed WDT, OS Event Timer
- Interfaces: 2x LPUART, 2x LPI2C, 2x LPSPI
- Security Blocks: PKC Engine, RNG, SHA512, AES-256, Protected Flash Region,
  2x Code Watchdog
- Analog: 16b ADC, Analog Comparator

Always-on Domain
================

- 10MHz Cortex-M0+
- 32KB RAM
- Interfaces: 1x LPUART, 1x LPI2C
- Analog: 16b ADC, Analog Comparator, Temperature Sensor
- Timers: RTC, LP Timers
- Segment LCD, Keypad

For more information about the MCX-L255 SoC and FRDM-MCXL255 board, see:

- `MCX L Series Website`_
- `MCX L Series Fact Sheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Targets available
==================

The default configuration enables the first core only.
CPU0 is the only target that can run standalone.
CPU1 is not supported.

Connections and IOs
===================

The MCX-L255 SoC has 4 gpio controllers and has pinmux registers which
can be used to configure the functionality of a pin.

+------------+-----------------+----------------------------+
| Name       | Function        | Usage                      |
+============+=================+============================+
| P2_10      | UART            | UART RX cpu0               |
+------------+-----------------+----------------------------+
| P2_11      | UART            | UART TX cpu0               |
+------------+-----------------+----------------------------+
| P0_7       | UART            | UART RX cpu1               |
+------------+-----------------+----------------------------+
| P0_6       | UART            | UART TX cpu1               |
+------------+-----------------+----------------------------+

System Clock
============

The MCX-L255 SoC is configured to use FRO 96M (FIRC) running at 96MHz as
a source for the system clock.

Serial Port
===========

The FRDM-MCXL255 SoC has 3 LPUART interfaces. The default console is LPUART0.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the MCU-Link CMSIS-DAP Onboard Debug Probe.

Using LinkServer
----------------

Linkserver is the default runner for this board, and supports the factory
default MCU-Link firmware. Follow the instructions in
:ref:`mcu-link-cmsis-onboard-debug-probe` to reprogram the default MCU-Link
firmware. This only needs to be done if the default onboard debug circuit
firmware was changed. To put the board in ``ISP mode`` to program the firmware,
short jumper keep SW3 pressed during power-up or reset, then release it.

Using J-Link
------------

There are two options. The onboard debug circuit can be updated with Segger
J-Link firmware by following the instructions in
:ref:`mcu-link-jlink-onboard-debug-probe`.
To be able to program the firmware, you need to put the board in ``ISP mode``
by keeping SW3 pressed during power-up or reset, then release it.

The second option is to attach a :ref:`jlink-external-debug-probe` to the
10-pin SWD connector (J17) of the board.
For both options use the ``-r jlink`` option with west to use the jlink runner.

.. code-block:: console

   west flash -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to J16, and use the serial terminal of your choice
(minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxl255/mcxl255/cpu0
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-2208-gcf34f44c1e18 ***
   Hello World! frdm_mcxl255/mcxl255/cpu0

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_mcxl255/mcxl255/cpu0
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0-2208-gcf34f44c1e18 ***
   Hello World! frdm_mcxl255/mcxl255/cpu0

.. _MCX L Series Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/mcx-arm-cortex-m/mcx-l-series-microcontrollers:MCX-L-SERIES

.. _MCX L Series Fact Sheet:
   https://www.nxp.com/docs/en/fact-sheet/MCXLFS.pdf
