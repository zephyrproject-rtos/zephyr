.. zephyr:board:: mcb_rx26t

Overview
********

MCB-RX26T Type B is a CPU board for motor control evaluation. By using this product in combination
with an inverter board, motor control using RX26T can be easily performed.

**MCU Native Pin Access**

The MCB-RX26T Type B includes:

- CPU maximum operating frequency 120MHz, 32-bit RXv3 Core
- Package/Pin count: LFQFP/100 pin
- ROM/RAM: 512KB/64KB
- MCU input clock: 10MHz (Generate with external crystal oscillator)

**System Control and Debugging**

- Power supply: DC 5V,3.3V (selectable with jumper switch) Select one way automatically from
  the below

  - USB-powered (debug port)
  - External power supply via standard input

- Debugging support:

  - Via E2lite debugger with E2OB (Onboard debugger circuit) - open Jumper J11
  - Via JLink with JTAG connector - short Jumper J11

- Connector:

  - Inverter board connector
  - USB connector for E2 OB
  - SCI connector for Renesas Motor Workbench communication
  - Through hole for CAN communication
  - Through hole for SPI communication
  - PMOD connectors

- User LEDs and buttons:

  - Four User LEDs
  - Power LED indicating availability of regulated power
  - One Reset button

Hardware
********
Detailed hardware features can be found at:

- RX26T MCU: `RX26T Group User's Manual Hardware`_

- MCB-RX26T Type B: `MCB-RX26T Type B - User's Manual`_

Note:
The CPU used in the RX26T is based on the RXv3 core. However, the current version of the Zephyr
kernel only supports the RXv1 core. Since the RXv3 core is backward-compatible with RXv1, it works
with this version. But the following limitations apply:

- FPU context saving is not supported. Do not use the FPU.
- Register bank save function is not supported. Do not use instructions for register bank save function.
- Accumulator register saving is not supported. Do not use DSP instructions or any libraries that
  include DSP instructions.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``mcb_rx26t`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

**Note:** Currently, the RX26T is built and programmed using the Renesas GCC RX toolchain.
Please follow the steps below to program it onto the board:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path/to/your/toolchain>/bin/rx-elf-

  - Build the Blinky Sample for EK-RX26T

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b mcb_rx26t@typeb samples/basic/blinky

Flashing
========

Program can be flashed to MCB-RX26T via e2lite E2OB (Onboard debugger circuit).

To flash the program to board

  1. Connect from board's debug connector port to host PC using USB connector for E2 OB.

  2. Execute west command

   .. code-block:: console

      west flash

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
                "device": "R5F526TF",
                "debuggerType": "E2LITE",
                "serverParameters": [
                    "-uUseFine=", "1",
                    "-w=", "1",
                ],
            }
        }
    ]
  }


References
**********

- `MCB-RX26T Type B Website`_
- `RX26T MCU group Website`_

.. _MCB-RX26T Type B Website:
   https://www.renesas.com/en/design-resources/boards-kits/mcb-rx26t-type-b

.. _RX26T MCU group Website:
   https://www.renesas.com/en/products/microcontrollers-microprocessors/rx-32-bit-performance-efficiency-mcus/rx26t-32-bit-microcontroller-optimized-dual-motor-and-pfc-control

.. _MCB-RX26T Type B - User's Manual:
   https://www.renesas.com/en/document/mat/mcb-rx26t-type-b-users-manual

.. _RX26T Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx26t-group-users-manual-hardware

.. _Renesas Debug extension:
   https://marketplace.visualstudio.com/items?itemName=RenesasElectronicsCorporation.renesas-debug
