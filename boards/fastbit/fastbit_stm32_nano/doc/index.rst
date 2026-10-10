.. zephyr:board:: fastbit_stm32_nano

Overview
********

The Fastbit STM32 Nano is a compact development board based on the
STM32F303CCT6 microcontroller (ARM Cortex-M4 with FPU). It integrates a
Bosch BMI2xx motion sensor, a USB Type-C connector with an on-board CH340N
USB-to-UART bridge and three 2x5 expansion headers. The
:ref:`Fastbit Nano LCD Shield <fastbit_nano_lcd>`, with a 1.28" round LCD,
a touch controller and a microSD card slot, plugs onto these headers.

Hardware
********

- STM32F303CCT6 in LQFP48 package
- ARM Cortex-M4 CPU with FPU, up to 72 MHz
- 256 KB flash, 40 KB SRAM and 8 KB core-coupled memory (CCM)
- 8 MHz external crystal (HSE)
- USB Type-C connector with a slide switch (SW4) that routes the USB data
  lines either to the MCU USB peripheral or to the on-board CH340N UART bridge
- Bosch BMI2xx 6-axis motion sensor on I2C1 at address 0x68 (a BMI270 or a
  BMI260, depending on the production batch; the IMU is not enabled in the
  devicetree yet)
- Three 2x5 expansion headers (P1, P2 and P3)
- Three user LEDs, one power LED, a user button, a reset button and a BOOT0
  button
- External SWD header for programming and debugging

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- USART1_TX : PA9
- USART1_RX : PA10
- I2C1_SCL : PB6
- I2C1_SDA : PB7
- SPI1_SCK : PA5
- SPI1_MISO : PA6
- SPI1_MOSI : PA7
- SPI2_SCK : PB13
- SPI2_MISO : PB14
- SPI2_MOSI : PB15
- USER_PB : PA0
- LED1 (blue) : PA1
- LED2 (green) : PA2
- LED3 (red) : PA3

Other board signals that are wired but not yet described in the devicetree:

- IMU_INT : PA8
- USB_DM : PA11
- USB_DP : PA12

Expansion Headers
=================

The headers P1, P2 and P3 are described by the ``nano_header`` connector node.
The parent pin index of a header pin is ``(header number - 1) * 10 +
(header pin number - 1)``, and the power pins are not mapped. The buses of the
headers are available under the labels ``nano_spi1`` (SPI1 on P1),
``nano_spi2`` (SPI2 on P2), ``nano_i2c1`` (I2C1 on P3) and ``nano_uart1``
(USART1 on P2). See :dtcompatible:`fastbit,nano-header` for the complete pin
list. The Fastbit Nano LCD Shield uses this interface.

System Clock
============

The system clock is driven by the main PLL at 72 MHz, fed by the 8 MHz
external crystal.

Serial Port
===========

The Zephyr console is assigned to USART1 (115200 8N1). USART1 is connected to
the on-board CH340N when the SW4 slide switch is in the UART position, so the
console is available through the USB Type-C connector as a virtual COM port.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board has no on-board debug probe. Connect an external ST-LINK/V2 (or
compatible) probe to the 6-pin SWD header (SWCLK, SWDIO, NRST, GND, 3V3).

Applications for the ``fastbit_stm32_nano`` board can be built and flashed in
the usual way (see :ref:`build_an_application` and :ref:`application_run` for
more details).

Flashing
========

The board is configured to be flashed using the west `STM32CubeProgrammer`_
runner, so its :ref:`installation <stm32cubeprog-flash-host-tools>` is
required.

Alternatively, OpenOCD or J-Link can also be used to flash the board using the
``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to the Fastbit STM32 Nano
-------------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: fastbit_stm32_nano
   :goals: build flash

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: fastbit_stm32_nano
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Fastbit STM32 Nano website:
   https://fastbitembedded.com/products/fastbit-stm32-nano-v2-1-with-1-28-tft-lcd-display

.. _Fastbit STM32 Nano schematic:
   https://cdn.shopify.com/s/files/1/0835/2592/7199/files/fastbit_stm32_nano_sch_v2_1_SPI_LCD.pdf

.. _Fastbit STM32 Nano user manual:
   https://cdn.shopify.com/s/files/1/0835/2592/7199/files/fastbit_stm32_nano_UM_v2_1.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
