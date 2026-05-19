.. zephyr:board:: fpb_ra0e1

Overview
********

The FPB-RA0E1 board is designed with an architecture similar to other boards in the FPB series. Alongside
the RA MCU there is an on-board programmer, pin headers for access to all the pins on the RA MCU, a
power supply regulator, some LEDs and switches, and several ecosystem I/O connectors (Pmod and
Arduino).

The key features of the FPB-RA0E1 board are categorized in two groups (consistent with the architecture of
the board) as follows:

**MCU and MCU Native Pin Access**

- R7FA0E1073CFJ MCU (referred to as RA MCU)
- 32 MHz, Arm® Cortex®-M23 core
- 64 KB Code Flash, 12 KB SRAM, 1 KB Data Flash
- 32-pin, LQFP package
- Native pin access through 2 x 16-pin male headers (not fitted)
- MCU’s VCC current measurement point for precision current consumption measurement
- Multiple clock sources – Oscillators for high-speed, medium-speed, and low-speed on-chip clock signals
  are available in the RA MCU. Signals from crystal oscillators at 20.000 MHz (not fitted) and 32.768 kHz
  can also be used for the main clock and the sub-clock, respectively

**System Control and Ecosystem Access**

- USB Full Speed Device (USB 2.0 Type-C™ connector)

- Two 5 V input sources

   - USB (Debug, Full Speed)
   - External power supply (using 2-pin header) (not fitted)

- On-board debugger (SWD)

- User LEDs and buttons

   - User LEDs (green) x 2
   - Power LED (green) indicating availability of regulated power
   - Debug/power LED (yellow) indicating power and the debug connection
   - User button x 1
   - Reset button x 1

- Two popular ecosystem expansions

   - Digilent PmodTM (SPI, UART, and I2C) connectors x 2
   - Arduino® (Uno R3) connectors

Hardware
********

Detailed hardware features can be found at:

- RA0E1 MCU: `RA0E1 Group User's Manual Hardware`_
- FPB-RA0E1 board: `FPB-RA0E1 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_ra0e1`` board configuration can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fpb_ra0e1
   :goals: flash

Open a serial terminal, reset the board (press the reset switch S2), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.3.0-xxx-xxxxxxxxxxxxx *****
   Hello World! fpb_ra0e1/r7fa0e1073cfj

Flashing
========

Program can be flashed to FPB-RA0E1 via the on-board SEGGER J-Link debugger.
SEGGER J-link's drivers are available at https://www.segger.com/downloads/jlink/

To flash the program to board

1. Connect to J-Link OB via USB port to host PC

2. Make sure J-Link OB jumper is in default configuration as describe in `FPB-RA0E1 - User's Manual`_

3. Execute west command

	.. code-block:: console

		west flash -r jlink

References
**********

- `FPB-RA0E1 Website`_
- `RA0E1 MCU group Website`_

.. _FPB-RA0E1 Website:
   https://www.renesas.com/en/design-resources/boards-kits/fpb-ra0e1

.. _RA0E1 MCU group Website:
   https://www.renesas.com/en/products/ra0e1

.. _FPB-RA0E1 - User's Manual:
   https://www.renesas.com/en/document/mat/fpb-ra0e1-users-manual

.. _RA0E1 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/ra0e1-group-users-manual-hardware
