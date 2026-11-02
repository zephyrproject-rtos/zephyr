.. zephyr:board:: stm32h7p5i_dk

Overview
********

The STM32H7P5I-DK Discovery Kit is an evaluation board featuring the
STM32H7P5IIK6Q microcontroller. It provides a range of interfaces and
peripherals for evaluating the STM32H7P5 series.

More information about the board can be found on the `STM32H7P5I-DK website`_.

Hardware
********

The STM32H7P5xx SoC provides the following high-level capabilities used by this
board configuration:

- STM32H7P5IIK6Q microcontroller based on the Arm® Cortex®-M7 core at 520 MHz,
  featuring 2 Mbytes of dual-bank flash memory and 736 Kbytes of SRAM, in an
  UFBGA176+25 package
- Internal SMPS to generate Vcore logic supply
- 0.96", 128 x 64-pixel OLED display
- USB Type-C® Sink device FS
- One MEMS digital microphone
- 512 Mbits Octo-SPI NOR flash memory
- Three user LEDs
- Three push-buttons (user, reset, and boot mode)
- One potentiometer for ADC
- Board connectors:

  - M.2 Key E connectivity connector
  - ST-LINK USB Type-C® connector
  - User USB Type-C® connector
  - microSD™ card holder
  - MIPI10 debug in connector
    (Arm® Cortex® 10-pin 1.27 mm-pitch debug connector over STDC14 footprint)
  - ARDUINO® Uno V3 expansion connector
  - Motor control V2 connector
  - mikroBUS™ connector

- Flexible power-supply options: ST-LINK USB VBUS, user USB connector, or external sources
- On-board STLINK-V3EC debugger/programmer with USB re-enumeration capability:
  mass storage, Virtual COM port, and debug port

More information about STM32H7P5xx devices is available at:

- `STM32H7P5II on www.st.com`_
- `STM32H7P Reference Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

- ST-Link Virtual COM Port: USART2_RX/TX on PA3/PA2
- Arduino Serial: UART4_RX/TX on PH14/PH13
- User button: PC13
- User LEDs:
  - Green LED (LD1): PH6
  - Red LED (LD2): PG1
  - Blue LED (LD3): PH5

For more details, please refer to the STM32H7P5I-DK board User Manual.

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

The board includes an STLINK-V3EC debugger/programmer.

Applications for the ``stm32h7p5i_dk`` board can be built and flashed in
the usual way (see :ref:`build_an_application` and :ref:`application_run`
for more details).

Flashing
========

The board is configured to be flashed using west's `STM32CubeProgrammer`_ runner,
so :ref:`installing the tool <stm32cubeprog-flash-host-tools>` is required.

Flashing an application to STM32H7P5I-DK
----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32h7p5i_dk
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way using the
:ref:`ST-LINK GDB Server <runner_stlink_gdbserver>`.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32h7p5i_dk
   :goals: debug

References
**********

.. target-notes::

.. _STM32H7P5I-DK website:
   https://www.st.com/en/evaluation-tools/stm32h7p5i-dk.html

.. _STM32H7P5II on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h7p5ii.html

.. _STM32H7P Reference Manual:
   https://www.st.com/resource/en/reference_manual/rm0533-stm32h7pxxx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
