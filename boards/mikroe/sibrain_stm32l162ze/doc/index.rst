.. zephyr:board:: mikroe_sibrain_stm32l162ze

Overview
********

The SiBRAIN for STM32L162ZE (also sold as MCU CARD 13 for STM32 STM32L162ZE,
MIKROE-4647) is a MikroElektronika add-on board that carries a STMicroelectronics
STM32L162ZE ultra-low-power Arm Cortex-M3 microcontroller running at up to 32 MHz,
with 512 KB of Flash memory, 80 KB of SRAM and 16 KB of data EEPROM.

The board follows the MikroElektronika SiBRAIN (formerly MCU Card) standard and
plugs into a host development board through two 168-pin mezzanine connectors.
It has no power supply, debugger, USB-UART, LEDs or buttons on its own; these are
provided by the host board, for example Fusion for ARM v8, Fusion for STM32 v8,
UNI-DS v8, Mikromedia 7 or UNI Clicker. For more information see the
`SiBRAIN for STM32L162ZE website`_.

Hardware
********

- STM32L162ZE in LQFP144 package
- 24 MHz crystal (HSE)
- 32.768 kHz crystal (LSE)
- Two 168-pin SiBRAIN connectors routing all MCU ports, mikroBUS 1-5,
  display, USB-UART and programming/trace signals to the host board

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The system clock is derived from the 24 MHz HSE crystal through the PLL
(96 MHz VCO, 32 MHz SYSCLK). The RTC is clocked from the 32.768 kHz LSE crystal.

The Zephyr console is on UART5 (PC12 TX, PD2 RX). These pins are the SiBRAIN
USB-UART pins, which the host board routes to its USB-UART converter.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

The SWD interface is routed through the SiBRAIN connector to the programmer
of the host board.

Building & Flashing
===================

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mikroe_sibrain_stm32l162ze
   :goals: build flash

References
**********

.. target-notes::

.. _SiBRAIN for STM32L162ZE website:
   https://www.mikroe.com/mcu-card-13-for-stm32-stm32l162ze
