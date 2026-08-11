.. zephyr:board:: nucleo_g564re

Overview
********

The Nucleo G564RE board is an affordable development platform for the
STMicroelectronics STM32G564RE Arm Cortex-M33 microcontroller.

The STM32 Nucleo-64 board provides an affordable and flexible way
for users to try out new concepts and build prototypes by choosing from
the various combinations of performance and power consumption features
provided by the STM32 microcontroller.

More information about the board can be found at the `Nucleo G564RE website`_.

Hardware
********

The STM32G564xx SoC provides the following high-level capabilities used by this
board configuration:

- STM32 microcontroller in an LQFP64 or LQFP48 package
- 1 user LED
- Three push-buttons: user, reset, and boot
- 32.768 kHz crystal oscillator
- Flexible power-supply options: ST-LINK USB VBUS or external sources
- External or internal SMPS to generate Vcore logic supply
- 24 MHz or 48 MHz HSE
- User USB Device full speed, or USB SNK/UFP full speed
- Cryptography
- CAN FD transceiver
- Board connectors:

  - ARDUINO® Uno V3 expansion connector
  - ST morpho extension pin headers for full access to all STM32 I/Os
  - External SMPS experimentation dedicated connector
  - USB Type-C®, Micro-B, or Mini-B connector for the ST-LINK
  - USB Type-C® user connector
  - MIPI® debug connector
  - CAN FD header

- On-board ST-LINK (ST-LINK/V2-1, STLINK-V3E, STLINK-V2EC, or STLINK-V3EC) debugger/programmer with
  USB re-enumeration capability: mass storage, Virtual COM port, and debug port


More information about STM32G564 devices is available at:

- `STM32G564RE on www.st.com`_
- `STM32G5 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr peripheral mapping:

- Console (LPUART1): PA2 (TX), PA3 (RX)
- User LED (LD1): PG3
- User button: PC13

For connector-level details, refer to the board user manual.

System Clock
------------

The default configuration uses a 24 MHz HSE clock and PLL settings
(``M=3, N=40, R=2``) to run the system at 160 MHz.

RTC is enabled and sourced from LSE (32.768 kHz), matching the external
LSE crystal present on the board schematic.

Serial Port
-----------

The Zephyr console is configured on LPUART1 at 115200 8N1 by default.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Nucleo G564RE board includes an STLINK-V3EC embedded debug tool interface.

Applications for the ``nucleo_g564re`` board can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to Nucleo G564RE
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_g564re
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way using the
:ref:`ST-LINK GDB Server <runner_stlink_gdbserver>`.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_g564re
   :goals: debug

References
**********

.. target-notes::

.. _Nucleo G564RE website:
	https://www.st.com/en/evaluation-tools/nucleo-g564re.html

.. _STM32G564RE on www.st.com:
	https://www.st.com/en/microcontrollers-microprocessors/stm32g564re.html

.. _STM32G5 reference manual:
	https://www.st.com/resource/en/reference_manual/rm0514-stm32g5-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
	https://www.st.com/en/development-tools/stm32cubeprog.html
