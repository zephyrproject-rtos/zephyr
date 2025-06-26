.. zephyr:board:: rsk_rx130

Overview
********

The Renesas Starter Kit for RX130-512KB is the perfect starter kit for
developers who are new to the RX130 (Program Flash 512KB, Pin Count 100-pin),
which operates at up to 32 MHz and is based on the RXv1 core architecture,
making it suitable for various embedded applications

**MCU Native Pin Access**

The RSKRX130-512KB includes:

- 32-MHz, 32-bit RX MCUs in 100 pins LFQFP package, Micon Pin Headers
- Direct MCU pin access through standard headers for easy peripheral integration
- Internal high-speed oscillator and low-speed on-chip oscillators
- Three low power consumption modes

**System Control and Debugging**

- USB Full-Speed Device (mini-B connector) for communication and power

- Power source options:

  - USB-powered (debug port)
  - External power supply via standard input

- Debugging support:

  - Via Jlink debugger with RX adapter boards.

- User LEDs and buttons:

  - Four User LEDs (red x2, yellow, green)
  - Power LED (green) indicating availability of regulated power
  - One Reset button, three User buttons

- Ecosystems expansions:

  - Two Digilent Pmod (LCD and Spare) connectors
  - 2Kbit I2C EEPROM

**Special Feature Access**

- IEC60730 compliance
- Capacitive touch sensing unit
- LCD drive capability for displaying data or status in real-time applications

Hardware
********
Detailed hardware features can be found at:

- RX130 MCU: `RX130 Group User's Manual Hardware`_
- RSK-RX130-512KB: `RSK_RX130_512KB - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``rsk_rx130@512kb`` board target can be built, flashed, and
debugged in the usual way. See :ref:`build_an_application` and
:ref:`application_run` for more details on building and running.

If you want to build Zephyr application for RSK-RX130 board using Renesas GCC RX toolchain follow
the steps below:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path to your toolchain>/bin/rx-elf-

  - Build the Blinky Sample for RSK-RX130-512KB:

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b rsk_rx130@512kb samples/basic/blinky

Flashing
========

Program can be flashed to RSKRX130-512KB using Jlink with RX adapter boards, by
connecting the board's debug connector port to the host PC. Here's an example
for building and flashing the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rsk_rx130@512kb
   :goals: build flash

Debugging
=========

You can use `Renesas Debug extension`_ on Visual Studio code for a visual debug interface.
The configuration for launch.json is as below.

.. code-block:: json

  {
    "version": "0.2.0",
    "configurations": [
        {
            "type": "renesas-hardware",
            "request": "launch",
            "name": "Renesas GDB Hardware Debugging",
            "target": {
                "deviceFamily": "RX",
                "device": "R5F51308",
                "debuggerType": "SEGGERJLINKRX",
            }
        }
    ]
  }


References
**********

- `RSK_RX130_512KB Website`_
- `RX130 MCU group Website`_

.. _RSK_RX130_512KB Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/rx-32-bit-performance-efficiency-mcus/rx130-512kb-starter-kit-renesas-starter-kit-rx130-512kb

.. _RX130 MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/rx-32-bit-performance-efficiency-mcus/rx130-cost-optimized-high-performance-32-bit-microcontroller-enhanced-touch-key-function-and-5v-operation

.. _RSK_RX130_512KB - User's Manual:
   https://www.renesas.com/en/document/mat/renesas-starter-kit-rx130-512kb-users-manual-rev100

.. _RX130 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx130-group-users-manual-hardware-rev300

.. _Renesas Debug extension:
   https://marketplace.visualstudio.com/items?itemName=RenesasElectronicsCorporation.renesas-debug
