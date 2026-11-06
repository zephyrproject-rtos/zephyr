.. zephyr:board:: nucleo_h7p5ii

Overview
********

The Nucleo H7P5II Nucleo-144 board provides an affordable and flexible way
for users to try out new concepts and build prototypes by choosing from the
various combinations of performance and power-consumption features provided
by the STM32H7Pxxx microcontroller.

More information about the board can be found on the `Nucleo H7P5II website`_.

Hardware
********

The STM32H7P5xx SoC provides the following high-level capabilities used by this
board configuration:

- STM32H7P5IIK6 microcontroller based on the Arm® Cortex®-M7 processor, featuring
  2 Mbytes of dual bank flash memory and 736 Kbytes of SRAM, including TCM memories,
  in a UFBGA201 package
- Internal LDO to generate Vcore logic supply
- USB Type-C® UFP sink device FS
- Three user LEDs
- Reset, user, and boot mode push-buttons
- 24 MHz HSE crystal oscillator
- 32.768 kHz LSE crystal oscillator
- Board connectors:
  - USB Type-C® connector
  - FDCAN connector
  - ARDUINO® Uno V3 connector
  - ST morpho extension pin headers for full access to most of the STM32 I/Os
  - M.2 Key A serial memory connector
- Flexible power-supply options: ST-LINK USB VBUS, USB connector, or external sources
- Onboard STLINK-V3EC debugger/programmer with USB re-enumeration capability: mass storage, Virtual
  COM port, and debug port

More information about STM32H7P5xx devices is available:

- `STM32H7P5II on www.st.com`_
- `STM32H7Pxxx Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- ST-Link Virtual COM Port: USART2_RX/TX on PA3/PA2
- Arduino Serial: UART4_RX/TX on PD11/PD12
- User button: PC13
- User LEDs:
  - Green LED (LD1): PG9
  - Red LED (LD2): PF1
  - Blue LED (LD3): PF4

For more details, please refer to the `Nucleo H7P5II board User Manual`_.

System Clock
------------

The default configuration uses the 24 MHz HSE as input to PLL1, which is
configured to generate a 520 MHz system clock.

Serial Port
-----------

The Zephyr console is configured on USART2 at 115200 8N1 by default.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Nucleo H7P5II includes an STLINK-V3EC debugger/programmer.

Applications for the ``nucleo_h7p5ii`` board can be built and flashed in
the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

The board is configured to be flashed using west's `STM32CubeProgrammer`_ runner,
so :ref:`installing the tool <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to Nucleo H7P5II
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky
    :board: nucleo_h7p5ii
    :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way using the
:ref:`ST-LINK GDB Server <runner_stlink_gdbserver>`.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_h7p5ii
   :goals: debug

References
**********

.. target-notes::

.. _Nucleo H7P5II website:
   https://www.st.com/en/evaluation-tools/nucleo-h7p5ii.html

.. _STM32H7P5II on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h7p5ii.html

.. _STM32H7Pxxx Reference Manual:
   https://www.st.com/resource/en/reference_manual/rm0533-stm32h7pxxx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _Nucleo H7P5II board User Manual:
   https://www.st.com/resource/en/user_manual/um2808-nucleo-h7p5ii-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
