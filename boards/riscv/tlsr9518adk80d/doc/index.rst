.. _tlst9518adk80d:

Telink TLSR9518ADK80D
#####################

Overview
********

The TLSR9518A Generic Starter Kit is a hardware platform which
can be used to verify the `Telink TLSR9 series chipset`_ and develop applications
for several 2.4 GHz air interface standards including Bluetooth 5.2 (Basic data
rate, Enhanced data rate, LE, Indoor positioning and BLE Mesh),
Zigbee 3.0, Homekit, 6LoWPAN, Thread and 2.4 Ghz proprietary.

.. figure:: img/tlsr9518adk80d.jpg
     :width: 400px
     :align: center
     :alt: TLSR9518ADK80D

More information about the board can be found at the `Telink B91 Generic Starter Kit Hardware Guide`_ website.

Hardware
********

The TLSR9518A SoC integrates a powerful 32-bit RISC-V MCU, DSP, AI Engine, 2.4 GHz ISM Radio, 256
KB SRAM (128 KB of Data Local Memory and 128 KB of Instruction Local Memory), external Flash memory,
stereo audio codec, 14 bit AUX ADC, analog and digital Microphone input, PWM, flexible IO interfaces,
and other peripheral blocks required for advanced IoT, hearable, and wearable devices.

.. figure:: img/tlsr9518_block_diagram.jpg
     :width: 400px
     :align: center
     :alt: TLSR9518ADK80D_SOC

The TLSR9518ADK80D default board configuration provides the following hardware components:

- RF conducted antenna
- 1 MB External Flash memory with reset button
- Chip reset button
- Mini USB interface
- 4-wire JTAG
- 4 LEDs, Key matrix up to 4 keys
- 2 line-in function (Dual Analog microphone supported when switching jumper from microphone path)
- Dual Digital microphone
- Stereo line-out

Supported Features
==================

The Zephyr TLSR9518ADK80D board configuration supports the following hardware features:

+----------------+------------+------------------------------+
| Interface      | Controller | Driver/Component             |
+================+============+==============================+
| PLIC           | on-chip    | interrupt_controller         |
+----------------+------------+------------------------------+
| RISC-V Machine | on-chip    | timer                        |
| Timer (32 KHz) |            |                              |
+----------------+------------+------------------------------+
| PINMUX         | on-chip    | pinmux                       |
+----------------+------------+------------------------------+
| GPIO           | on-chip    | gpio                         |
+----------------+------------+------------------------------+
| UART           | on-chip    | serial                       |
+----------------+------------+------------------------------+

The following example projects are supported:

- samples/hello_world
- samples/synchronization
- samples/philosophers
- samples/basic/threads
- samples/basic/blinky
- samples/basic/button
- samples/subsys/console/echo
- samples/subsys/console/getchar
- samples/subsys/console/getline
- samples/subsys/shell/shell_module
- samples/subsys/cpp/cpp_synchronization

.. note::
   To support "button" example project PC3-KEY3 (J20-19, J20-20) jumper needs to be removed and KEY3 (J20-19) should be connected to VDD3_DCDC (J51-13) externally.

   For the rest example projects use the default jumpers configuration.

Other hardware features and example projects are not supported yet.

Limitations
-----------

- Maximum 3 GPIO pins could be configured to generate interrupts simultaneously. All pins must be related to different ports and use different IRQ numbers.
- DMA mode is not supported by Serial Port.
- UART hardware flow control is not implemented.

Default configuration and IOs
=============================

System Clock
------------

The TLSR9518ADK80D board is configured to use the 24 MHz external crystal oscillator
with the on-chip PLL/DIV generating the 48 MHz system clock.
The following values also could be assigned to the system clock in the board DTS file
(``boards/riscv/tlsr9518adk80d/tlsr9518adk80d.dts``):

- 16000000
- 24000000
- 32000000
- 48000000
- 64000000
- 96000000

.. code-block::

   &cpu0 {
       clock-frequency = <48000000>;
   };

PINs Configuration
------------------

The TLSR9518A SoC has five GPIO controllers (PORT_A to PORT_E), but only two are
currently enabled (PORT_B for LEDs control and PORT_C for buttons) in the board DTS file:

- LED0 (blue): PB4, LED1 (green): PB5, LED2 (white): PB6, LED3 (red): PB7
- Key Matrix SW0: PC2_PC3, SW1: PC2_PC1, SW2: PC0_PC3, SW3: PC0_PC1

Peripheral's pins on the SoC are mapped to the following GPIO pins in the
``boards/riscv/tlsr9518adk80d/tlsr9518adk80d.dts`` file:

- UART0 RX: PB2, TX: PB3
- UART1 RX: PC6, TX: PC7

Serial Port
-----------

The TLSR9518A SoC has 2 UARTs. The Zephyr console output is assigned
to UART0 in the ``boards/riscv/tlsr9518adk80d/tlsr9518adk80d_defconfig`` file.
The default settings are 115200 8N1.

Programming and debugging
*************************

Building
========

You can build applications in the usual way. Here is an example for
the "hello_world" application.

.. code-block:: console

   # From the root of the zephyr repository
   west build -b tlsr9518adk80d samples/hello_world

To use `Telink RISC-V Linux Toolchain`_, ``ZEPHYR_TOOLCHAIN_VARIANT`` and ``CROSS_COMPILE`` variables need to be set.

.. code-block:: console

   # Set Zephyr toolchain variant to cross-compile
   export ZEPHYR_TOOLCHAIN_VARIANT=cross-compile
   # Specify the Telink RISC-V Toolchain location
   export CROSS_COMPILE=~/toolchains/nds32le-elf-mculib-v5f/bin/riscv32-elf-
   # From the root of the zephyr repository
   west build -b tlsr9518adk80d samples/hello_world

`Telink RISC-V Linux Toolchain`_ is available on the `Burning and Debugging Tools for TLSR9 Series in Linux`_ page.

Open a serial terminal with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flash the board, reset and observe the following messages on the selected
serial port:

.. code-block:: console

   *** Booting Zephyr OS version 2.5.0  ***
   Hello World! tlsr9518adk80d


Flashing
========

In order to flash the TLSR9518ADK80D board check the following resources:

- `Burning and Debugging Tools for all Series`_
- `Burning and Debugging Tools for TLSR9 Series`_
- `Burning and Debugging Tools for TLSR9 Series in Linux`_

Debugging
=========

Supporting UART debug and OpenOCD+GDB.

References
**********

.. target-notes::

.. _Telink TLSR9 series chipset: http://wiki.telink-semi.cn/wiki/chip-series/TLSR9-Series/
.. _Telink B91 Generic Starter Kit Hardware Guide: http://wiki.telink-semi.cn/wiki/Hardware/B91_Generic_Starter_Kit_Hardware_Guide/
.. _Telink RISC-V Linux Toolchain: http://wiki.telink-semi.cn/tools_and_sdk/Tools/IDE/telink_riscv_linux_toolchain.zip
.. _Burning and Debugging Tools for all Series: http://wiki.telink-semi.cn/wiki/IDE-and-Tools/Burning-and-Debugging-Tools-for-all-Series/
.. _Burning and Debugging Tools for TLSR9 Series: http://wiki.telink-semi.cn/wiki/IDE-and-Tools/Burning-and-Debugging-Tools-for-TLSR9-Series/
.. _Burning and Debugging Tools for TLSR9 Series in Linux: http://wiki.telink-semi.cn/wiki/IDE-and-Tools/BDT_for_TLSR9_Series_in_Linux/
