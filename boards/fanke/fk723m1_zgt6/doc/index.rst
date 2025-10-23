.. zephyr:board:: fk723m1_zgt6

Overview
********

The FK723M1-ZGT6 board is a development board for the STM32H723ZGT6 SoC.

Key Features:

- STM32 microcontroller in LQFP144 package
- USB OTG or full-speed device
- 1 user LEDs
- 1 boot and reset push-buttons
- 15 MHz and 32.768 kHz crystal oscillators

Board connectors:

- USB with USB-C
- FPC10P LCD connector
- FPC20P Camera connector
- 8 pin debug connector

More information about the board can be found at the `FK723M1-ZGT6 Schematic`_.

Hardware
********

FK723M1-ZGT6 provides the following hardware components:

- STM32H723ZG in LQFP144 package
- ARM 32-bit Cortex-M7 CPU with FPU
- Chrom-ART Accelerator
- Hardware JPEG Codec
- 550 MHz max CPU frequency
- VDD from 1.62 V to 3.6 V
- 1 MB Flash
- 562 kB SRAM max (376 kb used currently)
- High-resolution timer (2.1 ns)
- 32-bit timers(2)
- 16-bit timers(12)
- SPI(6)
- I2C(4)
- I2S (3)
- USART(4)
- UART(4)
- USB OTG Full Speed(1)
- CAN FD(2)
- SAI(2)
- SPDIF_Rx(4)
- HDMI_CEC(1)
- Dual Mode Quad SPI(1)
- Camera Interface
- GPIO (up to 114) with external interrupt capability
- 16-bit ADC(3) with 36 channels / 3.6 MSPS
- 12-bit DAC with 2 channels(2)
- True Random Number Generator (RNG)
- 16-channel DMA
- LCD-TFT Controller with XGA resolution

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

The FK723M1-ZGT6 board features one USB port, two 30x2 pin headers, one 4x2 debug header,
one micro SD slot, one FPC10P LCD interface, one FPC20P Camera interface and one built-in external Quad SPI flash.
The board is configured as follows:

- UART_1 TX/RX : PA9/PA10 (debug header UART)
- LD1 : PG7

System Clock
------------

FK723M1-ZGT6 System Clock could be driven by an internal or external
oscillator, as well as the main PLL clock. By default, the System clock is
driven by the PLL clock at 550MHz, driven by an 15MHz high-speed external clock.

Serial Port
-----------

FK723M1-ZGT6 board has 4 UARTs and 4 USARTs. The Zephyr console output is
assigned to UART1. Default settings are 115200 8N1.

Programming and Debugging
*************************

FK723M1-ZGT6 provides a special SWD header.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to FK723M1-ZGT6
----------------------------------------

First, connect a SWD capable debugger to the debug header on FK723M1-ZGT6.
Then connect the debugger to the host computer to prepare the board for flashing.
Finally, build and flash your application.

Here is an example for the :zephyr:code-sample:`blinky` application.

Optional: Connect a USB-to-serial adapter to RX and TX (cross connect!).

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

or use screen:

.. code-block:: console

   $ screen /dev/ttyACM0 115200

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: fk723m1_zgt6
   :goals: build flash

You should see the following messages on the console repeatedly:

.. code-block:: console

   $ LED state: ON
   $ LED state: OFF

Hello World example can also be used:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fk723m1_zgt6
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: fk723m1_zgt6
   :maybe-skip-config:
   :goals: debug

.. _FK723M1-ZGT6 Schematic:
   https://community.st.com/ysqtg83639/attachments/ysqtg83639/mcu-boards-hardware-tools-forum/20009/1/FK723M1-ZGT6.zh-CN.en.pdf

.. _STM32H723ZG on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h723zg.html

.. _STM32H723 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00603761-stm32h723733-stm32h725735-and-stm32h730-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeIDE:
   https://www.st.com/en/development-tools/stm32cubeide.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
