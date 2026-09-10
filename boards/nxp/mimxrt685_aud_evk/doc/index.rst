.. zephyr:board:: mimxrt685_aud_evk

Overview
********

The i.MX RT600 is a crossover MCU family optimized for 32-bit immersive audio
playback and voice user interface applications combining a high-performance
Cadence Tensilica HiFi 4 audio DSP core with a next-generation Cortex-M33
core. The i.MX RT600 family of crossover MCUs is designed to unlock the
potential of voice-assisted end nodes with a secure, power-optimized embedded
processor.

The MIMXRT685-AUD-EVK board is an audio-focused evaluation kit for the
i.MX RT685 crossover MCU, featuring a CS42448 audio codec and multiple
audio interfaces.

Hardware
********

- MIMXRT685SFVKB Cortex-M33 (300 MHz, 4.5 MB on-chip SRAM) core processor with Cadence Xtensa HiFi4 DSP
- Onboard, high-speed USB, Link2 debug probe with CMSIS-DAP protocol (supporting Cortex M33 debug only)
- High speed USB port with micro A/B connector for the host or device functionality
- UART, I2C and SPI port bridging from i.MX RT685 target to USB via the on-board debug probe
- 64 MB Macronix Quad SPI Flash operating at 1.8 V
- NXP PCA9420UK PMIC
- User LEDs
- Reset and User buttons
- CS42448 audio codec

For more information about the MIMXRT685 SoC and MIMXRT685-AUD-EVK board, see
these references:

- `i.MX RT685 Website`_
- `i.MX RT685 Datasheet`_
- `i.MX RT685 Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The MIMXRT685 SoC has IOCON registers, which can be used to configure the
functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_2  | USART           | USART RX                   |
+---------+-----------------+----------------------------+
| PIO0_1  | USART           | USART TX                   |
+---------+-----------------+----------------------------+
| PIO0_14 | GPIO            | GREEN LED                  |
+---------+-----------------+----------------------------+
| PIO0_26 | GPIO            | BLUE LED                   |
+---------+-----------------+----------------------------+
| PIO0_31 | GPIO            | RED LED                    |
+---------+-----------------+----------------------------+
| PIO1_1  | GPIO            | SW1                        |
+---------+-----------------+----------------------------+
| PIO0_10 | GPIO            | SW2                        |
+---------+-----------------+----------------------------+

System Clock
============

The MIMXRT685-AUD-EVK is configured to use the OS Event timer
as a source for the system clock.

Serial Port
===========

The MIMXRT685 SoC has 8 FLEXCOMM interfaces for serial communication. One is
configured as USART for the console and the remaining are not used.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the LPC-Link2.

.. tabs::

    .. group-tab:: LinkServer CMSIS-DAP

        1. Install the :ref:`linkserver-debug-host-tools` and make sure they are in your
           search path.  LinkServer works with the default CMSIS-DAP firmware included in
           the on-board debugger.

        linkserver is the default runner for this board

        .. code-block:: console

           west flash
           west debug

    .. group-tab:: JLink

        1. Install the :ref:`jlink-debug-host-tools` and make sure they are in your search path.

        .. code-block:: console

           west flash -r jlink
           west debug -r jlink

Configuring a Console
=====================

Connect a USB cable from your PC to the debug USB port, and use the serial
terminal of your choice (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`linkserver-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt685_aud_evk/mimxrt685s/cm33
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.x.x *****
   Hello World! mimxrt685_aud_evk/mimxrt685s/cm33

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application. This example uses the
:ref:`linkserver-debug-host-tools` as default.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mimxrt685_aud_evk/mimxrt685s/cm33
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v4.x.x *****
   Hello World! mimxrt685_aud_evk/mimxrt685s/cm33

.. include:: ../../common/board-footer.rst.inc

.. _i.MX RT685 Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus/i-mx-rt600-crossover-mcu-with-arm-cortex-m33-and-dsp-cores:i.MX-RT600

.. _i.MX RT685 Datasheet:
   https://www.nxp.com/docs/en/data-sheet/RT600.pdf

.. _i.MX RT685 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=UM11147
