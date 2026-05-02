.. zephyr:board:: weact_stm32g030_core

Overview
********
The WeAct Studio STM32G030 Core Board provides an affordable and flexible way for
users to try out new concepts and build prototypes with the STM32G030F6
microcontroller. This compact development board is designed for space-constrained
applications while maintaining essential functionality.

The board requires an external ST-LINK or compatible SWD programmer
for flashing and debugging, as it does not include an onboard debugger.

Hardware
********
WeAct STM32G030 provides the following hardware components:

- STM32G030F6P6 microcontroller in TSSOP-20 package featuring 32 Kbytes of Flash
  memory and 8 Kbytes of SRAM
- Compact form factor optimized for embedded applications
- Exposed SWD header for programming and debugging (SWDIO, SWCLK, GND, 3V3)
- One user LED connected to PA4 (blue LED)
- NRST button for manual reset
- Flexible board power supply:

  - External 3.3V supply via power pins
  - 5V input with onboard regulator

- All GPIO pins broken out for maximum flexibility

More information about STM32G030F6 can be found in the
`STM32G0x0 reference manual`_ and `STM32G030x6 datasheet`_.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain),
as input (with or without pull-up or pull-down), or as peripheral alternate function.
Most of the GPIO pins are shared with digital or analog alternate functions. All GPIOs
are high current capable except for analog inputs.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PB6/PB7 (Primary Console)
- UART_2 TX/RX : PA2/PA3 (Secondary UART)
- I2C2 SCL/SDA : PA11/PA12
- SPI1 SCK/MISO/MOSI : PA1/PA6/PA7
- User LED : PA4 (Blue LED)
- SWD Interface : PA13 (SWDIO), PA14 (SWCLK)

System Clock
------------

The WeAct STM32G030 board is configured to use the internal HSI oscillator at 16 MHz
with PLL to generate a system clock of 64 MHz. The board also includes LSE crystal
support for RTC applications.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

WeAct STM32G030 board requires an external ST-LINK/V2 or compatible SWD debugger for
programming and debugging. Connect your debugger to the SWD header:

- SWDIO (PA13)
- SWCLK (PA14)
- GND
- 3V3 (optional, for powering the board)

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to WeAct STM32G030
-------------------------------------------

Connect your ST-LINK or compatible programmer to the SWD header on the board.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32g030_core
   :goals: build flash

You will see the blue LED on PA4 blinking every second.

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: weact_stm32g030_core
   :maybe-skip-config:
   :goals: debug

Serial Console
==============

The primary serial console is available on USART1 (PB6/PB7) at 115200 baud. Connect
a USB-to-serial adapter to these pins to access the Zephyr shell and console output:

- TX: PB6
- RX: PB7
- GND: GND

References
**********

.. target-notes::

.. _STM32G0x0 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0454-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32G030x6 datasheet:
   https://www.st.com/resource/en/datasheet/stm32g030c6.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
