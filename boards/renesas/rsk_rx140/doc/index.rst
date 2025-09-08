.. zephyr:board:: rsk_rx140

Overview
********

The Renesas Starter Kit for RX140 is an evaluation and starter kit for developers who are new
to the RX140 MCU family (Program Flash 256KB, RAM 64KB, Pin Count 80-pin).
The kit includes an LCD display module and an on-chip debugging emulator

**MCU Native Pin Access**

The RSK-RX140 includes:

- 48-MHz, 32-bit RX140 MCU (R5F51406BDFN, 80-pin LFQFP package)
- Direct MCU pin access through standard headers for easy peripheral integration
- On-board 8 MHz crystal, 32.768 kHz sub-clock, and internal oscillators
- Multiple low power consumption modes

**System Control and Debugging**

- USB Mini-B connector for serial communication (via on-board RL78/G1C USB-to-Serial MCU)
- Power source options:

  - USB-powered
  - External DC supply (5V input jack)
  - Debugger supply (E2 Lite)

- Debugging support:

  - Via E2 Lite debugger (14-pin connector)

- User LEDs and buttons:

  - Four User LEDs (green, orange, red x2)
  - Power LED (green)
  - One Reset button, three User buttons
  - One potentiometer (connected to ADC input)

- Ecosystem expansions:

  - Two Digilent Pmod connectors (LCD and Spare)
  - On-board 2Kbit I2C EEPROM

**Special Feature Access**

- Capacitive touch sensing (slider x1, buttons x2)
- CAN and LIN transceivers
- IEC60730 compliance support
- Security functions (built-in Trusted Secure IP)

Hardware
********
Detailed hardware features can be found at:

- RX140 MCU: `RX140 Group User's Manual Hardware`_
- RSK-RX140: `RSK_RX140 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``rsk_rx140`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

**Note:** Currently, the RX140 is built and programmed using the Renesas GCC RX toolchain.
Please follow the steps below to program it onto the board:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path/to/your/toolchain>/bin/rx-elf-

  - Build the Blinky Sample for RSK-RX140

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b rsk_rx140 samples/basic/blinky

Flashing
========

The program can be flashed to RSK-RX140 using the **E2 Lite debugger** by
connecting the board's 14-pin debug connector to the host PC.
Here’s an example for building and flashing the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rsk_rx140
   :goals: build flash

Debugging
=========

You can use `Renesas Debug extension`_ on Visual Studio Code for a visual debug interface.
The configuration for launch.json is as below.

.. code-block:: json

  {
    "version": "0.2.0",
    "configurations": [
        {
            "type": "renesas-hardware",
            "request": "launch",
            "name": "RX140 Renesas Debugging E2lite",
            "target": {
                "deviceFamily": "RX",
                "device": "R5F51406",
                "debuggerType": "E2LITE"
                "serverParameters": [
                    "-uUseFine=", "1",
                    "-w=", "0",
                ],
            }
        }
    ]
  }

References
**********

- `RSK_RX140 Website`_
- `RX140 MCU group Website`_

.. _RSK_RX140 Website:
   https://www.renesas.com/en/design-resources/boards-kits/rsk-rx140

.. _RX140 MCU group Website:
   https://www.renesas.com/en/products/rx140

.. _RSK_RX140 - User's Manual:
   https://www.renesas.com/en/document/mat/renesas-starter-kit-rx140-users-manual

.. _RX140 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx140-group-users-manual-hardware-rev120

.. _Renesas Debug extension:
   https://marketplace.visualstudio.com/items?itemName=RenesasElectronicsCorporation.renesas-debug
