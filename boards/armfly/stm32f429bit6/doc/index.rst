.. Copyright (c) 2026 Manyi Yu
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: armfly_stm32f429bit6

Overview
********

The Armfly STM32F429BIT6 board is a development board based on the
STMicroelectronics STM32F429BIT6 microcontroller in an LQFP-208 package.
It combines the STM32F429 MCU with external SDRAM, display and touch
hardware, multiple storage devices, dual Ethernet interfaces, audio,
sensors, and a large number of board-level expansion interfaces.

The current Zephyr board port provides initial support for the board. The
upstream description currently focuses on the base MCU, external SDRAM,
USART1 console, LTDC display path, GT911 touch controller, SPI NOR flash,
MicroSD socket, Ethernet MAC and PHY path, RTC, keys, and optional
board-private FMC latch GPIO outputs used for the onboard LEDs.

For more information about the SoC, see:

- `STM32F429 product page`_
- `STM32F429 reference manual`_

Hardware
********

The full Armfly STM32F429BIT6 board includes the following hardware:

- STM32F429BIT6 MCU in an LQFP-208 package
- ARM Cortex-M4 core with FPU
- Up to 180 MHz CPU frequency
- 2 MB internal flash
- 192 KB internal SRAM
- 16 MB SDRAM connected over a 32-bit FMC bus
- 128 MB NAND flash connected over an 8-bit FMC bus
- 8 MB SPI NOR flash
- 16 KB serial EEPROM
- MicroSD card socket
- RGB888 LCD interface with I2C touch and PWM-adjustable backlight
- Dual Ethernet interfaces:

  - One interface using the STM32 internal MAC with a DM9161 PHY
  - One DM9000AE controller connected on an FMC bus

- One USB full-speed host port
- One USB full-speed device port
- Two CAN 2.0 interfaces
- One RS-232 interface
- One RS-485 interface
- One MicroSD card socket
- WM8978 full-duplex I2S audio codec
- Si4704 AM/FM radio receiver
- One PS/2 keyboard or mouse connector
- Infrared receive and transmit hardware
- One buzzer shared with the infrared transmit function
- MPU6050 motion sensor
- BH1750 ambient light sensor
- BMP180 barometric pressure sensor
- Three user buttons, one 5-way joystick
- Four user LEDs
- CR1220 battery holder for RTC backup

The Zephyr board port currently provides only initial support. Many peripherals
present on the full board are not yet modeled or validated in the upstream
board description.

Supported Features
==================

.. zephyr:board-supported-hw::

The current board definition enables or describes the following hardware blocks:

- STM32F429BIT6 SoC core support
- Internal flash and SRAM
- External SDRAM on FMC
- USART1 console
- GPIO keys
- Optional board-private FMC latch GPIO controller for the onboard LEDs
- I2C1 with GT911 touch controller, MPU6050, BH1750, BMP180 sensors, and
  AT24 EEPROM
- SDMMC1 for the MicroSD socket
- LTDC display controller using external SDRAM
- PWM-based LCD backlight control
- CAN1 interface

.. note::

   The onboard LEDs are connected through an external latch on FMC Bank1. They
   are disabled by default because the FMC controller must be enabled by the
   application before accessing the latch. Applications that use these LEDs must
   enable MEMC support and enable the latch GPIO node in an application overlay.

Pin Mapping
===========

Default Zephyr peripheral mapping:

- USART1 TX/RX: PA9 / PA10
- USART3 TX/RX: PB10 / PB11
- I2C1 SCL/SDA: PB6 / PB9
- GT911 INT: PH7
- MPU6050 INT: PE4
- SPI1 SCK/MISO/MOSI: PA5 / PA6 / PA7
- SPI1 CS: PF11 / PG11 / PC3
- SDMMC1 D0-D3/CK/CMD: PC8 / PC9 / PC10 / PC11 / PC12 / PD2
- SDMMC1 card detect: PE2
- CAN1 RX/TX: PA11 / PA12
- USB OTG HS DM/DP: PB14 / PB15
- PWM backlight control: TIM5 CH1 on PA0
- Buttons: BUTTON0=PI8, BUTTON1=PC13, BUTTON2=PH4
- Joystick: DOWN=PF10, SELECT=PI11, RIGHT=PG7, UP=PG2, LEFT=PG3
- LED0-3: latch GPIO bits 8-11 (active low)

System Clock
============

The board is populated with a 8 MHz high-speed external oscillator and a
32.768 kHz low-speed external crystal for RTC operation. The current Zephyr
board port uses the RCC and PLL configuration described in devicetree for
system and peripheral clocks.

Display and Touch
=================

The board DTS enables the STM32 LTDC controller for an RGB888 display path
(800x480) and assigns the external ``SDRAM1`` memory region as framebuffer
backing storage. The GT911 touch controller is connected on ``I2C1`` and
selected as the board touch device through ``zephyr,touch``.

The LVGL pointer pseudo-device is intentionally not part of the base board DTS.
Applications that use LVGL can provide it in an overlay when needed.

Serial Port
===========

The default Zephyr console is assigned to USART1 with 115200 8N1 settings.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``armfly_stm32f429bit6`` board can be built and flashed in
the usual way. The board definition provides runner support for
STM32CubeProgrammer, OpenOCD, J-Link, pyOCD, and probe-rs.

Flashing
========

The default runner is `STM32CubeProgrammer`_. OpenOCD, J-Link, pyOCD, and
probe-rs can also be selected explicitly:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink
   $ west flash --runner pyocd
   $ west flash --runner probe-rs

Example build and flash command for the :zephyr:code-sample:`hello_world`
application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: armfly_stm32f429bit6
   :goals: build flash

Debugging
=========

Debugging can be started in the usual way:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: armfly_stm32f429bit6
   :goals: debug

.. _STM32F429 product page:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f429-439.html

.. _STM32F429 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00031020.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
