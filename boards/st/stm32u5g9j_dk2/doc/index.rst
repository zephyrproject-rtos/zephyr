.. zephyr:board:: stm32u5g9j_dk2

Overview
********

The STM32U5G9J-DK2 Discovery kit is a complete demonstration and development
platform for the STM32U5G9ZJT6Q microcontroller, featuring an Arm |reg| Cortex |reg|‑M33
core with Arm |reg| TrustZone |reg|.

Leveraging the innovative ultra-low power-oriented features, 3 Mbytes of
embedded SRAM, 4 Mbytes of embedded flash memory, and rich graphics features,
the STM32U5G9J-DK2 Discovery kit enables users to prototype applications
with state-of-the-art energy efficiency, as well as providing stunning and
optimized graphics rendering with the support of a 2.5D Neo-Chrom accelerator,
chrom-ART Accelerator, and Chrom-GRC™ MMU.

The STM32U5G9J-DK2 Discovery kit integrates a full range of hardware features
that help the user evaluate all the peripherals, such as a 5" RGB 800x480 pixels
TFT colored LCD module with a 24‑bit RGB interface and capacitive touch panel,
USB Type-C |reg| HS, Octo‑SPI flash memory device, ARDUINO |reg|, and STLINK-V3EC
(USART console).

The STM32U5G9J-DK2 Discovery kit integrates an STLINK-V3EC embedded in-circuit
debugger and programmer for the STM32 microcontroller with a USB Virtual COM
port bridge and comes with the STM32CubeU5 MCU Package, which provides an STM32
comprehensive software HAL library as well as various software examples.

More information about the board can be found at the `STM32U5G9J-DK2 website`_.
More information about STM32U5G9ZJT6Q can be found here:

- `STM32U5G9ZJ on www.st.com`_
- `STM32U5 Series reference manual`_
- `STM32U5Gxxx datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

For more details please refer to `STM32U5G9J-DK2 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART_1 TX/RX : PA9/PA10 (ST-Link Virtual Port Com)
- USART_2 TX/RX : PA2/PA3
- LD2 : PD2
- LD3 : PD4
- User Button: PC13
- I2C1 SCL/SDA : PG14/PG13
- I2C2 SCL/SDA : PB10/PB11
- SPI1 SCK/MISO/MOSI/CS : PA5/PA6/PB5/PA3
- ADC1 : channel5 PA0, channel12 PA7
- ADC4 : channel4 PC3
- SDMMC1/LTDC conflicting pins: PC6, PC7, PC8, PC9, PB9
- SDMMC1_CK : PC12
- SDMMC1_CMD : PD2
- SDMMC1_D0 : PC8
- SDMMC1_D1 : PC9
- SDMMC1_D2 : PC10
- SDMMC1_D3 : PC11
- SDMMC1_D4 : PB8
- SDMMC1_D5 : PB9
- SDMMC1_D6 : PC6
- SDMMC1_D7 : PC7
- LTDC_R0 : PC6
- LTDC_R1 : PC7
- LTDC_R2 : PE15
- LTDC_R3 : PD8
- LTDC_R4 : PD9
- LTDC_R5 : PD10
- LTDC_R6 : PD11
- LTDC_R7 : PD12
- LTDC_G0 : PC8
- LTDC_G1 : PC9
- LTDC_G2 : PE9
- LTDC_G3 : PE10
- LTDC_G4 : PE11
- LTDC_G5 : PE12
- LTDC_G6 : PE13
- LTDC_G7 : PE14
- LTDC_B0 : PB9
- LTDC_B1 : PB2
- LTDC_B2 : PD14
- LTDC_B3 : PD15
- LTDC_B4 : PD0
- LTDC_B5 : PD1
- LTDC_B6 : PE7
- LTDC_B7 : PE8
- LTDC_DE : PD6
- LTDC_CLK : PD3
- LTDC_HSYNC : PE0
- LTDC_VSYNC : PD13

System Clock
============

The STM32U5G9J-DK Discovery 2 kit relies on an HSE oscillator (16 MHz crystal)
and an LSE oscillator (32.768 kHz crystal) as clock references.
Using the HSE (instead of HSI) is mandatory to manage the DSI interface for
the LCD module and the USB high‑speed interface.

Serial Port
===========

The STM32U5G9J Discovery 2 kit has up to 4 USARTs, 2 UARTs, and 1 LPUART.
The Zephyr console output is assigned to USART1 which connected to the onboard
ST-LINK/V3.0. Virtual COM port interface. Default communication settings are
115200 8N1.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32U5G9J Discovery 2 kit includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash and debug the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd

Flashing an application to STM32U5G9J_DK2
-----------------------------------------

Connect the STM32U5G9J Discovery 2 board to your host computer using the USB
port, then run a serial host program to connect with your Discovery
board. For example:

.. code-block:: console

   $ minicom -D /dev/ttyACM0 -b 115200

Then, build and flash in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32u5g9j_dk2
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32u5g9j_dk2

Debugging
=========

Default debugger for this board is openocd. It could be used in the usual way
with "west debug" command.
Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32u5g9j_dk2
   :goals: debug


.. _STM32U5G9J-DK2 website:
   https://www.st.com/en/evaluation-tools/stm32u5g9j-dk2.html

.. _STM32U5G9J-DK2 board User Manual:
   https://www.st.com/resource/en/user_manual/um3223-discovery-kit-with-stm32u5g9zj-mcu-stmicroelectronics.pdf

.. _STM32U5G9ZJ on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32u5g9zj.html

.. _STM32U5 Series reference manual:
   https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32U5Gxxx datasheet:
   https://www.st.com/resource/en/datasheet/stm32u5g7vj.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html

.. _STM32U5G9J_DK2 board schematics:
   https://www.st.com/resource/en/schematic_pack/mb1918-u5g9zjq-c01-schematic.pdf
