.. zephyr:board:: stm32h743_coreboard

Overview
********

The STM32H743IIT6 Core Board V1.3 is a development board based on the STM32H743IIT6
microcontroller, with external SDRAM, QSPI flash, an I2C EEPROM and a microSD card slot.

Key Features

- STM32 microcontroller in LQFP176 package
- 32 MB SDRAM (Winbond W9825G6KH-6I)
- 16 MB QSPI NOR flash (Winbond W25Q128JV)
- 2 Kbit I2C EEPROM (Microchip AT24C02C)
- microSD card slot
- 2 user LEDs
- 2 user and reset push-buttons
- 25 MHz and 32.768 kHz crystal oscillators
- Board connectors:

  - USB-C for USB OTG FS
  - USB-C for USART1, through a CH340X USB-UART bridge
  - SWD
  - RGB LCD, SPI LCD and DCMI camera
  - Two 50-pin headers

More information about the board can be found in the `board documentation`_.

Hardware
********

The STM32H743IIT6 Core Board provides the following hardware components:

- STM32H743IIT6 microcontroller in LQFP176 package:

  - Arm Cortex-M7 core with double-precision FPU, up to 480 MHz
  - 2 MB internal flash
  - 1 MB internal RAM: 128 KB DTCM, 64 KB ITCM, 864 KB user SRAM and 4 KB backup SRAM

- 25 MHz HSE crystal and 32.768 kHz LSE crystal
- Winbond W9825G6KH-6I 32 MB SDRAM on FMC SDRAM bank 1, 16-bit data bus
- Winbond W25Q128JV 16 MB NOR flash on QUADSPI bank 1
- Microchip AT24C02C 2 Kbit EEPROM on I2C2
- microSD card socket on SDMMC1, 4-bit bus, without card detect
- CH340X USB-UART bridge on USART1, behind the USB-C connector marked COM
- USB-C connector for USB OTG FS (PA11/PA12)
- User LEDs: LED0 (red) and LED1 (green), both active low, plus a power LED
- Push-buttons: KEY0, WK_UP and RESET
- 4-pin SWD header: 3.3 V, SWDIO (PA13), SWCLK (PA14), GND
- Connectors for an RGB LCD, an SPI LCD and an 18-pin DCMI camera
- Two 50-pin headers, P1 and P2, with most of the MCU I/O

The USB OTG FS port, the microSD socket, the LCD connectors and the camera connector are
not enabled in the board devicetree yet.

More information about the STM32H743II can be found here:

- `STM32H743II on www.st.com`_
- `STM32H743xI datasheet`_
- `STM32H743 reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Default Zephyr Peripheral Mapping:
----------------------------------

- USART1 TX/RX : PA9/PA10 (CH340X, console)
- I2C2 SCL/SDA : PH4/PH5 (EEPROM)
- QUADSPI CLK/BK1_NCS/BK1_IO0/BK1_IO1/BK1_IO2/BK1_IO3 : PB2/PB6/PF8/PF9/PF7/PF6
  (NOR flash)
- LED0 (red, ``led0``) : PB0
- LED1 (green, ``led1``) : PB1
- KEY0 (``sw0``) : PA1
- WK_UP (``sw1``) : PA0
- RTC : clocked by LSE

System Clock
------------

The system clock is driven by PLL1 at 480 MHz. PLL1 is fed by the 25 MHz HSE crystal
(M = 5, N = 192, P = 2). The AHB and AXI buses run at 240 MHz and all APB buses at
120 MHz. The FMC drives the SDRAM at 120 MHz (HCLK / 2). The RTC is clocked by the
32.768 kHz LSE crystal. The core is powered by the internal LDO.

Serial Port
-----------

The Zephyr console output is assigned to USART1, available on the USB-C connector
marked COM. Default settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

STM32H743IIT6 Core Board does not include an on-board debugger. An external SWD probe
has to be connected to the SWD header.

Flashing
========

The board is configured to be flashed and debugged using the west
:ref:`OpenOCD <openocd-debug-host-tools>` runner with an ST-Link probe.

Alternatively, `STM32CubeProgrammer`_ (with an ST-Link probe), J-Link or pyOCD can
be used with the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner stm32cubeprogrammer
   $ west flash --runner jlink
   $ west flash --runner pyocd

STM32CubeProgrammer and J-Link cannot connect while the core is in sleep mode, which
is the case when the application is idle. Applications flashed with these runners,
or debugged with OpenOCD, should enable
:kconfig:option:`CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP`.

Flashing an application to STM32H743IIT6 Core Board
---------------------------------------------------

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyUSB0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h743_coreboard
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! stm32h743_coreboard/stm32h743xx

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h743_coreboard
   :maybe-skip-config:
   :goals: debug

.. _board documentation:
   https://github.com/DaviVolcan/STM32H743IIT6-Core-Board

.. _STM32H743II on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h743ii.html

.. _STM32H743xI datasheet:
   https://www.st.com/resource/en/datasheet/stm32h743vi.pdf

.. _STM32H743 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
