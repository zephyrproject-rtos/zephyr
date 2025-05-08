.. zephyr:board:: fpb_rx261

Overview
********

The FPB-RX261 is a Fast Prototyping Board for the Renesas RX261 MCU Group, designed to
evaluate features of the RX261 family and accelerate embedded system development. The board
includes native MCU pin access, integrated debugger, power flexibility, and expansion connectors.

The key features of the FPB-RX261 board are categorized in two groups:

**MCU Native Pin Access**

- R5F52618BGFP MCU
- 64 MHz Renesas RXv3-based RX261 MCU in 100-pin LFQFP package
- 512 KB Code Flash, 8 KB Data Flash, 128 KB RAM
- Native pin access through 2 x 50-pin male headers (not fitted)
- MCU current measurement point for precision current consumption measurement
- RX MCU on-chip oscillators as main clock
- Providing 32.768 kHz crystal oscillator as sub clock

**System Control and Ecosystem Access**

- 5V input sources:

  - USB (USB DEBUG1 - J5)
  - External 5V via 2-pin header (J8)

- On-board debugger via E2 Emulator OB (FINE interface)

- User LEDs:

  - 2 Green User LEDs (PJ1, PC5)
  - 1 Green Power LED
  - 1 Yellow Debug LED

- User Buttons:

  - 1 User Switch (S1 - P32)
  - 1 Reset Switch (S2)

- Popular expansion connectors:

  - 2x Digilent Pmod connectors (J13, J14 - UART/SPI/I2C)
  - Arduino Uno R3-compatible header (J9-J12)

Hardware
********

Detailed hardware features can be found at:
- RX261 MCU: `RX261 Group User's Manual Hardware`_.
- FPB-RX261 board: `FPB-RX261 - User's Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Other hardware features are currently not supported by the port.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``fpb_rx261`` board can be built, flashed, and debugged using standard Zephyr workflows.
Refer to :ref:`build_an_application` and :ref:`application_run` for more details.

**Note:** Currently, the RX261 is built and programmed using the Renesas GCC RX toolchain.
Please follow the steps below to program it onto the board:

  - Download and install GCC for RX toolchain:

    https://llvm-gcc-renesas.com/rx-download-toolchains/

  - Set env variable:

   .. code-block:: console

      export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
      export CROSS_COMPILE=<Path/to/your/toolchain>/bin/rx-elf-

  - Build the Blinky Sample for FPB-RX261

   .. code-block:: console

      cd ~/zephyrproject/zephyr
      west build -p always -b fpb_rx261 samples/basic/blinky

Flashing
========

The program can be flashed to FPB-RX261 via the on-board **E2 Emulator OB**.

- **Renesas Flash Programmer (RFP)** can be used with the on-board debugger
  https://www.renesas.com/software-tool/renesas-flash-programmer-programming-gui

1. **Connect the board:**

   - Use USB DEBUG1 (J5) to connect the on-board E2 Emulator to the PC

2. **Configure jumpers for flashing:**

   - J4: Jumper on pins **2-3** (enable E2 OB)
   - J7: Jumper on pins **1-2** (3.3V system voltage)

3. **Run flashing command:**

   .. code-block:: console

      west flash

   Or use Renesas Flash Programmer with settings:
   - Debugger: E2 Emulator Lite
   - Interface: FINE
   - MCU: R5F52618

Debugging
=========

Debugging is supported via:

- **On-board E2 Emulator OB** using USB DEBUG1 (J5)

To use debugger:

- J4: Jumper on pins **2-3** (enable E2 OB)
- J7: Jumper on pins **1-2** (3.3V)
- LED4 (Yellow) indicates debugger status

**Note:** Only one FPB-RX261 board can be debugged per PC at a time using E2 OB.

References
**********
- `FPB-RX261 Website`_
- `RX261 MCU group Website`_

.. _FPB-RX261 Website:
   https://www.renesas.com/en/design-resources/boards-kits/fpb-rx261

.. _RX261 MCU group Website:
   https://www.renesas.com/en/products/rx261

.. _FPB-RX261 - User's Manual:
   http://renesas.com/en/document/mat/fpb-rx261-v1-users-manual?r=25565483

.. _RX261 Group User's Manual Hardware:
   https://www.renesas.com/en/document/mah/rx260-group-rx261-group-users-manual-hardware?r=25565707
