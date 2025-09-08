.. zephyr:board:: fpb_rx140

Overview
********

The Fast Prototyping Board for RX140 MCU Group comes equipped with an RX140 MCU
(R5F51406BGFN). The board is inexpensive for RX140 evaluation and prototype development of
various applications. It has an emulator circuit so you can write/debug programs just by
connecting it to a PC with a USB cable. In addition, it has high expandability with Arduino Uno
and two Pmod™ connectors as standard, and through-hole access to all pins of the
microcontroller.

**MCU Native Pin Access**

- R5F51406ADFN MCU

   - Max 48MHz, 32-bit RXv2 core (RXv2)
   - 256 KB Flash, 64 KB RAM
   - 80-pin, LFQFP package

- Native pin access through 2 x 40-pin male headers (not fitted)
- RX MCU current measurement point for precision current consumption measurement
- RX MCU on-chip oscillators as main clock
- Providing 32.768 kHz crystal oscillator as sub clock

**System Control and Ecosystem Access**

- Two 5 V input sources

   - USB
   - External power supply (using 2-pin header [not fitted])

- On-board debugger / programmer (E2 emulator On Board (referred as E2OB, FINE Interface))

- User LEDs and switches

   - Two User LEDs (green)
   - Power LED (green) indicating availability of regulated power
   - Debug LED (yellow) indicating the debug connection
   - One User switch
   - One Reset switch

- Two popular ecosystem expansions

   - Two Digilent PmodTM
   - Arduino® (Uno R3) connector

Hardware
********
Detailed hardware features can be found at:

- RX140 MCU: `RX140 Group User's Manual Hardware`_
- FPB-RX140: `FPB_RX140 - User's Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_rx140`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

**Note:** Currently, the RX140 is built and programmed using the Renesas GCC RX toolchain.
Please follow the steps below to program it onto the board:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path/to/your/toolchain>/bin/rx-elf-

  - Build the Blinky Sample for FPB-RX140

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b fpb_rx140 samples/basic/blinky

Flashing
========

The program can be flashed to RSK-RX140 using the **E2OB** by connecting the board to the host PC
and open Jumper J4.
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

- `FPB_RX140 Website`_
- `RX140 MCU group Website`_

.. _FPB_RX140 Website:
   https://www.renesas.com/en/design-resources/boards-kits/fpb-rx140

.. _RX140 MCU group Website:
   https://www.renesas.com/en/products/rx140

.. _FPB_RX140 - User's Manual:
   https://www.renesas.com/en/document/mat/fpb-rx140-v1-users-manual

.. _RX140 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx140-group-users-manual-hardware-rev120

.. _Renesas Debug extension:
   https://marketplace.visualstudio.com/items?itemName=RenesasElectronicsCorporation.renesas-debug
