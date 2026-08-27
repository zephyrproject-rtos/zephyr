.. Copyright (c) 2026 Zhaoming Li
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: armfly_stm32h743xih6

Overview
********

The Armfly STM32H743XIH6 board is a development board based on the
STMicroelectronics STM32H743XIH6 microcontroller in a TFBGA240 package.
It combines the STM32H743 MCU with external SDRAM, display and touch
hardware, multiple storage devices, dual Ethernet interfaces, audio,
sensors, and a large number of board-level expansion interfaces.

The current Zephyr board port provides initial support for the board. The
upstream description currently focuses on the base MCU, external SDRAM,
USART1 console, LTDC display path, GT911 touch controller, SPI NOR flash,
MicroSD socket, Ethernet MAC and PHY path, RTC, keys, and optional
board-private FMC latch GPIO outputs used for the onboard LEDs.

For more information about the SoC, see:

- `STM32H743 product page`_
- `STM32H743 reference manual`_

Hardware
********

The full Armfly STM32H743XIH6 board includes the following hardware:

- STM32H743XIH6 MCU in a TFBGA240 package
- ARM Cortex-M7 core with FPU
- Up to 480 MHz CPU frequency
- 2 MB internal flash
- 1 MB internal SRAM
- 32 MB SDRAM connected over a 32-bit FMC bus
- 32 MB QSPI flash with XIP capability
- 128 MB NAND flash connected over an 8-bit FMC bus
- 16 KB serial EEPROM
- 8 MB SPI NOR flash
- RGB888 LCD interface with I2C touch and adjustable backlight
- Dual Ethernet interfaces:

  - One interface using the STM32 internal MAC with a DM9162 PHY
  - One DM9000AEP controller connected on a 16-bit FMC bus

- One USB full-speed host port
- One USB full-speed device port
- One RS-485 interface
- One RS-232 interface with DB9 and TTL UART header access
- Two CAN interfaces
- One MicroSD card socket
- One SDIO expansion connector with two TTL serial ports
- WM8978 full-duplex I2S audio codec
- On-board speaker and microphone
- Si4704 FM radio receiver
- One PS/2 keyboard or mouse connector
- Infrared receive and transmit hardware
- One buzzer shared with the infrared transmit function
- MPU6050 motion sensor
- BH1750 ambient light sensor
- BMP180 barometric pressure sensor
- Three user buttons and one 5-way joystick
- One camera connector
- ADC and DAC expansion interface
- Expansion headers for SPI, TTL UART, and I2C modules compatible with the
  Armfly V5 and V6 expansion ecosystem, including support for:

  - AD7705 or TM7705 dual-channel 16-bit ADC modules
  - VS1053B audio modules
  - GPS and GPRS modules
  - ESP8266 UART Wi-Fi modules
  - OLED modules with an 8-bit parallel interface
  - AD7606 16-bit 8-channel ADC modules
  - ADS1256 8-channel 24-bit ADC modules
  - DAC8501 dual-channel DAC modules
  - DAC8563 dual-channel DAC modules
  - AD9833 DDS waveform generator modules

- Additional 5 V and 3.3 V expansion outputs implemented through a fast,
  FMC-connected 32-bit latch interface

The Zephyr board port currently provides only initial support. Many peripherals
present on the full board are not yet modeled or validated in the upstream
board description.

Supported Features
==================

.. zephyr:board-supported-hw::

The current board definition enables or describes the following hardware blocks:

- STM32H743XIH6 SoC core support
- Internal flash and SRAM
- External SDRAM on FMC
- USART1 console
- GPIO keys
- Optional board-private FMC latch GPIO controller for the onboard LEDs
- I2C1 with the GT911 touch controller, MPU6050 motion sensor, BH1750
  ambient light sensor, BMP180 pressure sensor, and AT24C128 EEPROM
- SPI3 with SPI NOR flash
- QUADSPI with QSPI NOR flash
- SDMMC1 for the MicroSD socket
- LTDC display controller using external SDRAM
- PWM-based LCD backlight control
- RTC using the low-speed clock source
- Ethernet MAC with MDIO and external PHY

.. note::

   The onboard LEDs are connected through an external latch on FMC Bank1. They
   are disabled by default because the FMC controller must be enabled by the
   application before accessing the latch. Applications that use these LEDs must
   enable MEMC support and enable the latch GPIO node in an application overlay.

Pin Mapping
===========

Default Zephyr peripheral mapping:

- USART1 TX/RX: PA9 / PA10
- I2C1 SCL/SDA: PB6 / PB9
- GT911 INT: PH7
- QUADSPI CLK/NCS/IO0/IO1/IO2/IO3: PF10 / PG6 / PF8 / PF9 / PF7 / PF6
- SPI3 SCK/MISO/MOSI: PB3 / PB4 / PB5
- SPI3 CS: PD13
- SDMMC1 D0-D3/CK/CMD: PC8 / PC9 / PC10 / PC11 / PC12 / PD2
- SDMMC1 card detect: PG12
- PWM backlight control: TIM1 CH1 on PA8
- Ethernet RMII REF_CLK/CRS_DV/TX_EN/TXD0/TXD1/RXD0/RXD1:
  PA1 / PA7 / PG11 / PG13 / PB13 / PC4 / PC5
- Ethernet MDC/MDIO: PC1 / PA2

System Clock
============

The board is populated with a 25 MHz high-speed external oscillator and a
32.768 kHz low-speed external crystal for RTC operation. The current Zephyr
board port uses the RCC and PLL configuration described in devicetree for
system and peripheral clocks.

Display and Touch
=================

The board DTS enables the STM32 LTDC controller for an RGB display path and
assigns the external ``SDRAM1`` memory region as framebuffer backing storage.
The GT911 touch controller is connected on ``I2C1`` and selected as the board
touch device through ``zephyr,touch``.

The LVGL pointer pseudo-device is intentionally not part of the base board DTS.
Applications that use LVGL can provide it in an overlay when needed.

I2C Devices
===========

The board DTS describes the onboard I2C devices connected to ``I2C1``:

- GT911 touch controller at address ``0x5d``
- MPU6050 motion sensor at address ``0x68``
- BH1750 ambient light sensor at address ``0x23``
- BMP180 pressure sensor at address ``0x77``
- AT24C128 EEPROM at address ``0x50``

Serial Port
===========

The default Zephyr console is assigned to USART1 with 115200 8N1 settings.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``armfly_stm32h743xih6`` board can be built and flashed in
the usual way. The board definition provides runner support for
STM32CubeProgrammer, OpenOCD, J-Link, and pyOCD.

Flashing
========

The default runner is `STM32CubeProgrammer`_. OpenOCD, J-Link, and pyOCD can
also be selected explicitly:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink
   $ west flash --runner pyocd

Example build and flash command for the :zephyr:code-sample:`hello_world`
application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: armfly_stm32h743xih6
   :goals: build flash

Debugging
=========

Debugging can be started in the usual way:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: armfly_stm32h743xih6
   :goals: debug

.. _STM32H743 product page:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h743-753.html

.. _STM32H743 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00314099.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
