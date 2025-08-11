.. zephyr:board:: ek_rx261

Overview
********

The EK-RX261 is an Evaluation Kit for the Renesas RX261 MCU Group, designed to evaluate features
of the RX261 family and accelerate embedded system development. The board includes rich
interfaces, peripheral connectors, and expansion support for popular ecosystem modules.

The key features of the EK-RX261 board are categorized in three groups:

**MCU Native Pin Access**

- RX261 MCU (R5F52618BGFP), 32-bit RXv3 core at 64MHz
- 512KB Code Flash, 8KB Data Flash, 128KB RAM
- Native pin access through 6 x 2-pin and 14 x 2-pin x 3-piece male headers
- MCU current measurement points for current consumption measurement
- Precision 8.000MHz main oscillator and 32.768kHz sub-clock

**System Control and Ecosystem Access**

- Four 5V input sources:

  - USB (USB DEBUG1, USB FULL SPEED, USB SERIAL)
  - External power supply (via TP5, TP6)

- Two Debug Modes:
  - Debug on-board via E2 Emulator OB (FINE interface)
  - Debug in via 14-pin external header (FINE interface)

- User LEDs:
  - 3 User LEDs (Red, Green, Blue)
  - 1 Power LED (White)
  - 1 Debug LED (Yellow)

- User Buttons:
  - 2 User Switches
  - 1 Reset Switch

- Popular ecosystem expansion connectors:

  - 2x Grove connectors (I2C / Analog)
  - 1x SparkFun Qwiic connector (I2C)
  - 2x Digilent Pmod connectors (UART/SPI/I2C)
  - Arduino Uno R3-compatible header
  - mikroBUS connector

- USB Full Speed Host and Device (micro-B connector)
- USB to Serial communication via FT234XD

**Special Feature Access**

- CAN FD transceiver with 3-pin header
- Touch interface (2 buttons)

Hardware
********

Detailed hardware features can be found at:
- RX261 MCU: `RX261 Group User's Manual Hardware`_.
- EK-RX261 board: `EK-RX261 - User's Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``ek_rx261`` board can be built, flashed, and debugged using standard
Zephyr workflows. Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

**Note:** Currently, the RX261 is built and programmed using the Renesas GCC RX toolchain.
Please follow the steps below to program it onto the board:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path/to/your/toolchain>/bin/rx-elf-

  - Build the Blinky Sample for EK-RX261

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b ek_rx261 samples/basic/blinky

Flashing
========

Program can be flashed to EK-RX261 via either the on-board **E2 Emulator OB** or
an external **SEGGER J-Link** debugger (using the FINE interface).

- **Renesas Flash Programmer (RFP)** can be used with the on-board E2 Emulator OB.
  RFP is available at: https://www.renesas.com/software-tool/renesas-flash-programmer-programming-gui

- **SEGGER J-Link** can be used with the 14-pin FINE connector (J27) for external flashing.
  J-Link drivers are available at: https://www.segger.com/downloads/jlink/

1. **Connect to the debugger:**

   - For E2 Emulator OB: connect **USB DEBUG1 (J26)**
   - For J-Link: connect to **14-pin FINE header (J27)**

2. **Set jumper configuration according to the selected flash mode:**

   - **E2 Emulator OB (on-board):**

     - J22: Open
     - J23: Jumper on pins **2-3**
     - J24: Open
     - J25: Open

   - **J-Link (external debugger):**

     - J22: Open
     - J23: Jumper on pins **1-2** or **2-3**
     - J24: **Closed**
     - J25: Open

3. **Run flashing command:**

   .. code-block:: console

      west flash  # For E2 Emulator OB
      west flash -r jlink     # For J-Link (external debugger)

Debugging
=========

The EK-RX261 supports debugging through:

- **On-board E2 Emulator OB (via USB DEBUG1)**
- **External debugger** using the 14-pin FINE header (J27)

To use on-board E2 Emulator:

- J22: Open
- J23: Jumper on pins **2-3**
- J24: Open
- J25: Open

To use external debugger:

- J24: Closed
- J22: Open
- J23: Jumper on pins 1-2 or 2-3 (depending on desired pull-up/down)
- J25: Open

**Note:** External debugger (e.g., E2 Lite) cannot power the board. Supply external 5V via USB or TP5/TP6.

References
**********
- `EK-RX261 Website`_
- `RX261 MCU group Website`_

.. _EK-RX261 Website:
   https://www.renesas.com/en/design-resources/boards-kits/ek-rx261

.. _RX261 MCU group Website:
   https://www.renesas.com/en/products/rx261

.. _EK-RX261 - User's Manual:
   https://www.renesas.com/en/document/mat/ek-rx261-v1-users-manual

.. _RX261 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx260-group-rx261-group-users-manual-hardware
