.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: skystar_stm32f407vet6

Overview
********

The SkyStar STM32F407VET6 board is a JLC core board based on the
STM32F407VET6 ARM Cortex-M4 MCU.

Hardware
********

Supported board features:

* STM32F407VET6 MCU
* 168 MHz system clock
* 512 KiB flash
* 192 KiB SRAM
* 8 MHz HSE crystal
* USB Type-C connector wired to USB device pins
* One user LED
* One user button
* USART1 exposed on the debug header

Programming and Debugging
*************************

Build the hello_world sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: skystar_stm32f407vet6
   :goals: build

Flashing
========

The board can be flashed with common STM32 runners such as J-Link,
OpenOCD, or dfu-util depending on the attached debug probe or boot mode.

References
**********

* `SkyStar core board documentation`_
* `SkyStar STM32F407VxT6 hardware documentation`_

.. _SkyStar core board documentation:
   https://wiki.lckfb.com/zh-hans/tkx/tkx-stm32f407vxt6/

.. _SkyStar STM32F407VxT6 hardware documentation:
   https://wiki.lckfb.com/zh-hans/tkx/hardware/schematic.html
