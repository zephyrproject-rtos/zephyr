.. zephyr:board:: emsbc_neon_cm7

Overview
********

The emSBC-Neon-CM7 enables a wide diversity of applications taking benefit
from high-speed connectivity, multimedia and sensor interface features.
Important board features include:

- STM32F769NIH6 microcontroller featuring 2 Mbytes of internal Flash memory and 512 Kbytes of internal SRAM
- 16 MByte Quad-SPI Flash memory
- 128-MBit SDRAM
- Connector for microSD card
- USB 2.0 Host
- USB 2.0 Device with Micro-AB connector
- 10/100 Ethernet interface compliant with IEEE-802.3-2002
- 18 bpp RGB-LCD display interface
- SAI audio interface
- GPIOs
- JTAG connector for debugging
- Two push-buttons (wakeup and reset)
- Arduino Uno V3 connectors
- Power supply output for external applications: 3.3V or 5V
- 7..24VDC power supply input

More information about the board can be found at the `emSBC-NEON-CM7 website`_.

Hardware
********

The emSBC-NEON-CM7 provides the following hardware components:

- STM32F769NIH6 in BGA216 package
- ARM® 32-bit Cortex®-M7 CPU with FPU
- 216 MHz max CPU frequency
- 2 MB Flash
- 512 + 16 + 4 KB SRAM
- Chrom-ART Accelerator(DMA2D), graphical hardware accelerator enabling enhanced graphical user interface
- Hardware JPEG codec
- LCD-TFT controller supporting up to XGA resolution
- MIPI®  DSI host controller supporting up to 720p 30Hz resolution
- 3x12-bit ADC with 24 channels
- 2x12-bit D/A converters
- General Purpose Timers
- Watchdog Timers
- True random number generator
- CRC calculation unit
- RTC: sub-second accuracy, hardware calendar
- 96-bit unique ID
- SDIO socket
- 10/100 Ethernet
- USB 2.0 high-speed Host
- USB 2.0 high-speed Device
- CAN
- 2x UART
- 18 bpp RGB LCD interface
- SAI
- 2x SPI
- I2C
- 6x GPIO

More information about STM32F769NIH6 can be found here:

- `STM32F769NIH6 on www.st.com`_
- `STM32F76xxx reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

emSBC-NEON-CM7 has 9 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `emSBC-NEON-CM7 Hardware Manuals`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_2 TX/RX/RTS/CTS : PD5/PD6/PD4/PD3
- UART_3 TX/RX : PB10/PB11 (Arduino Serial)
- I2C1 SCL/SDA : PB8/PB7 (Arduino I2C)
- SPI2 NSS/SCK/MISO/MOSI: PI0/PI1/PC2/PI3 (Arduino SPI)
- SPI5 SCK/MISO/MOSI: PH6/PH7/PF9
- QUADSPI CLK/NCS/IO0/IO1/IO2/IO3: PF10/PB6/PF8/PD12/PF7/PF6
- ETH REFCLK/CRSDV/MDIO/MDC/RXD0/RXD1/TXEN/TXD0/TXD1: PA1/PA7/PA2/PC1/PC4/PC5/PG11/PB12/PB13
- ETH PHY RESET: PC0
- SDIO CD#/CMD/CK/D0/D1/D2/D3: PA10/PD2/PC12/PC8/PC9/PC10/PC11 (micro SDC)
- CAN TX/RX: PB9/PI9
- LD1: PI7
- ADC1 IN9: PB1
- ADC2 IN6: PA6
- ADC3 IN3: PA3
- DAC1 OUT1/OUT2: PA4/PA5

System Clock
============

The STM32F769I System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock is driven by the PLL
clock at 216MHz, driven by a 25MHz high speed external clock.

Serial Port
===========

The Zephyr console output is assigned to UART2 which connected to the DEBUG
console interface of the emSBC-NEON-CM7. Default communication
settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``emsbc_neon_cm7`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink

Flashing an application to emSBC-NEON-CM7
-------------------------------------------

First, connect the emSBC-NEON-CM7 to your host computer using an ST/Link-V2
to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: emsbc_neon_cm7@R3
   :goals: build flash

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message on the console:

.. code-block:: console

   Hello World! arm

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: emsbc_neon_cm7@R3
   :goals: debug


.. _emSBC-NEON-CM7 website:
   https://www.emtrion.de/produkt/emsbc-neon-cm7-mit-stm32f769ni-von-stmicroelectronics/

.. _emSBC-NEON-CM7 Hardware Manuals:
   https://www.emtrion.de/produkt/emsbc-neon-cm7-mit-stm32f769ni-von-stmicroelectronics/#downloads

.. _STM32F769NIH6 on www.st.com:
	https://www.st.com/content/st_com/en/products/microcontrollers/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x9/stm32f769ni.html

.. _STM32F76xxx reference manual:
   https://www.st.com/resource/en/reference_manual/dm00224583.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
