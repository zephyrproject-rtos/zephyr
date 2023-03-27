.. _lpcxpresso55s36:

NXP LPCXpresso55S36
###################

Overview
********

The LPCXpresso55S36 board provides the ideal platform for evaluation
of the LPC55S3x/LPC553x MCU family, based on the Arm® Cortex®-M33
architecture. Arduino® UNO compatible shield connectors are included,
with additional expansion ports around the Arduino footprint, along
with a PMod/host interface port and MikroElektronika Click module
site.

.. image:: lpcxpresso55S36.jpg
   :align: center
   :alt: LPCXpresso55S36

Hardware
********

- LPC55S36 Arm® Cortex®-M33 microcontroller running at up to 150 MHz
- 256 KB flash and 96 KB SRAM on-chip
- LPC-Link2 debug high speed USB probe with VCOM port
- I2C and SPI USB bridging to the LPC device via LPC-Link2 probe
- MikroElektronika Click expansion option
- LPCXpresso expansion connectors compatible with Arduino UNO
- PMod compatible expansion / host connector
- Reset, ISP, wake, and user buttons for easy testing of software functionality
- Tri-color LED
- Full-speed USB device / host port
- High-speed USB device / host port
- UART header for external serial to USB cable
- CAN Transceiver
- Stereo audio codec with in/out line

For more information about the LPC55S36 SoC and LPCXPresso55S36 board, see:

- `LPC55S36 SoC Website`_
- `LPC55S36 Datasheet`_
- `LPC55S36 User Manual`_
- `LPCXpresso55S36 Website`_
- `LPCXpresso55S36 User Manual`_
- `LPCXpresso55S36 Development Board Design Files`_

Supported Features
==================

NXP considers the LPCXpresso55S36 as a superset board for the LPC55(S)3x
family of MCUs.  This board is a focus for NXP's Full Platform Support for
Zephyr, to better enable the entire LPC55(S)3x family.  NXP prioritizes enabling
this board with new support for Zephyr features.  The lpcxpresso55s36 board
configuration supports the hardware features below.  Another similar superset
board is the :ref:`lpcxpresso55s69`, and that board may have additional features
already supported, which can also be re-used on this lpcxpresso55s36 board:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| IOCON     | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| USART     | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| CAN       | on-chip    | canbus                              |
+-----------+------------+-------------------------------------+
| IAP       | on-chip    | flash                               |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| CSS       | on-chip    | entropy                             |
+-----------+------------+-------------------------------------+

Other hardware features are not currently enabled.

Currently available targets for this board are:

- *lpcxpresso55s36*

Connections and IOs
===================

The LPC55S36 SoC has IOCON registers, which can be used to configure
the functionality of a pin.

+---------+-----------------+----------------------------+
| Name    | Function        | Usage                      |
+=========+=================+============================+
| PIO0_17 | GPIO            | USR SW3                    |
+---------+-----------------+----------------------------+
| PIO0_22 | GPIO            | GREEN LED                  |
+---------+-----------------+----------------------------+
| PIO0_28 | GPIO            | RED LED                    |
+---------+-----------------+----------------------------+
| PIO0_29 | USART           | USART RX                   |
+---------+-----------------+----------------------------+
| PIO0_30 | USART           | USART TX                   |
+---------+-----------------+----------------------------+
| PIO1_11 | GPIO            | BLUE_LED                   |
+---------+-----------------+----------------------------+
| PIO1_18 | GPIO            | Wakeup SW1                 |
+---------+-----------------+----------------------------+
| PIO1_20 | FLEXPPWM0_PWM0_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_17 | FLEXPPWM0_PWM0_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_6  | FLEXPPWM0_PWM1_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_22 | FLEXPPWM0_PWM1_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_8  | FLEXPPWM0_PWM2_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_4  | FLEXPPWM0_PWM2_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_21 | FLEXPPWM1_PWM0_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO0_3  | FLEXPPWM1_PWM0_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_23 | FLEXPPWM1_PWM1_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO0_21 | FLEXPPWM1_PWM1_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_25 | FLEXPPWM1_PWM2_A| pwm                        |
+---------+-----------------+----------------------------+
| PIO0_31 | FLEXPPWM1_PWM2_B| pwm                        |
+---------+-----------------+----------------------------+
| PIO1_2  | CAN0_TXD        | CAN TX                     |
+---------+-----------------+----------------------------+
| PIO1_3  | CAN0_RXD        | CAN RX                     |
+---------+-----------------+----------------------------+

System Clock
============

The LPC55S36 SoC is configured to use the internal FRO at 96MHz as a
source for the system clock. Other sources for the system clock are
provided in the SOC, depending on your system requirements.

Serial Port
===========

The LPC55S36 SoC has 8 FLEXCOMM interfaces for serial
communication. One is configured as USART for the console and the
remaining are not used.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application`
and :ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This
board is configured by default to use the LPC-Link2 CMSIS-DAP Onboard
Debug Probe, however the :ref:`pyocd-debug-host-tools` does not yet
support the LPC55S36 so you must reconfigure the board for one of the
J-Link debug probe instead.

First install the :ref:`jlink-debug-host-tools` and make sure they are
in your search path.

Then follow the instructions in
:ref:`lpclink2-jlink-onboard-debug-probe` to program the J-Link
firmware. Please make sure you have the latest firmware for this
board.

Configuring a Console
=====================

Connect a USB cable from your PC to J1 (LINK2), and use the serial
terminal of your choice (minicom, putty, etc.) with the following
settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso55s36
   :goals: flash

Open a serial terminal, reset the board (press the RESET button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v2.2.0 *****
   Hello World! lpcxpresso55s36

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: lpcxpresso55s36
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS zephyr-v2.2.0 *****
   Hello World! lpcxpresso55s36

.. _LPC55S36 SoC Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/lpc5500-cortex-m33/lpc553x-s3x-advanced-analog-armcortex-m33-based-mcu-family:LPC553x

.. _LPC55S36 Datasheet:
    https://www.nxp.com/docs/en/data-sheet/LPC553x.pdf

.. _LPC55S36 User Manual:
   https://www.nxp.com/docs/en/reference-manual/LPC553xRM.pdf

.. _LPCxpresso55S36 Website:
   https://www.nxp.com/design/development-boards/lpcxpresso-boards/development-board-for-the-lpc553x-family-of-mcus:LPCXpresso55S36

.. _LPCXpresso55S36 User Manual:
   https://www.nxp.com/docs/en/user-manual/LPCXpresso55S36UM.pdf

.. _LPCXpresso55S36 Development Board Design Files:
   https://www.nxp.com/webapp/Download?colCode=LPCXPRESSO5536_EVK-DESIGN-FILES
