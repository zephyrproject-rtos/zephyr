.. zephyr:board:: stm32f769i_disco

Overview
********

The discovery kit enables a wide diversity of applications taking benefit
from audio, multi-sensor support, graphics, security, security, video,
and high-speed connectivity features. Important board features include:

- STM32F769NIH6 microcontroller featuring 2 Mbytes of Flash memory and 512 Kbytes of RAM, in BGA216 package
- On-board ST-LINK/V2-1 supporting USB reenumeration capability
- USB ST-LINK functions: virtual COM port, mass storage, debug port
- Five power supply options:

  - ST LINK/V2-1
  - USB HS connector
  - 5 V from RJ45 (Power Over Ethernet)
  - 5 V from Arduino™ or external connector
  - USB charger

- 4-inch capacitive touch LCD display with MIPI-DSI connector
- SAI audio codec
- Two audio line jacks, one for input and one for output
- Stereo speaker outputs
- Four ST MEMS microphones on DFSDM inputs
- Two SPDIF RCA input and output connectors
- Two push-buttons (user and reset)
- 512-Mbit Quad-SPI Flash memory
- 128-Mbit SDRAM
- Connector for microSD card
- Wi-Fi or Ext-EEP daughterboard connector
- USB OTG HS with Micro-AB connector
- Ethernet connector compliant with IEEE-802.3-2002
- Power Over Ethernet based on IEEE 802.3af (Powered Device, 48 V to 5 V, 3 W)
- Power supply output for external applications: 3.3 V or 5 V
- Arduino Uno V3 connectors
- Comprehensive free software including a variety of examples, part of the STM32Cube package
- Supported by a wide choice of integrated development environments

More information about the board can be found at the `32F769I-DISCO website`_.

Hardware
********

The STM32F769I Discovery kit provides the following hardware components:

- STM32F769NIH6 in BGA216 package
- ARM |reg| 32-bit Cortex |reg| -M7 CPU with FPU
- 216 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 2 MB Flash
- 512 + 16 + 4 KB SRAM
- Flexible external memory controller with up to 32-bit data bus
- Dual mode Quad-SPI
- Chrom-ART Accelerator(DMA2D), graphical hardware accelerator enabling enhanced graphical user interface
- Hardware JPEG codec
- LCD-TFT controller supporting up to XGA resolution
- MIPI |reg|  DSI host controller supporting up to 720p 30Hz resolution
- 3x12-bit ADC with 24 channels
- 2x12-bit D/A converters
- DMA Controller
- General Purpose Timers (15)
- Watchdog Timers (2)
- I2C (4)
- USART/UART (8)
- SPI (6)
- SAI (2)
- CAN (3)
- SDMMC (2)
- SPDIFRX interface
- HDMI-CEC
- MDIO slave interface
- USB 2.0 full-speed device/host/OTG controller with on-chip PHY
- USB 2.0 high-speed/full-speed device/host/OTG controller with dedicated DMA, on-chip full-speed PHY and ULPI
- 10/100 Ethernet MAC with dedicated DMA: supports IEEE 1588v2 hardware, MII/RMII
- 8- to 14-bit camera interface up to 54 Mbyte/s
- True random number generator
- CRC calculation unit
- RTC: sub-second accuracy, hardware calendar
- 96-bit unique ID

More information about STM32F769NIH6 can be found here:

- `STM32F769NIH6 on www.st.com`_
- `STM32F76xxx reference manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Pin Mapping
===========

STM32F769I Discovery kit has 9 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `32F769I-DISCO board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PA9/PA10 (ST-Link Virtual Port Com)
- UART_6 TX/RX : PC6/PC7 (Arduino Serial)
- I2C1 SCL/SDA : PB8/PB9 (Arduino I2C)
- I2C4 SCL/SDA : PD12/PB7 (Touchscreen FT6202, PI13 Interrupt Pin)
- SPI2 SCK/MISO/MOSI : PA12/PB14/PB15 (Arduino SPI)
- ETH : PA1, PA2, PA7, PC1, PC4, PC5, PG11, PG13, PG14
- LD1 : PJ13
- LD2 : PJ5
- LD3 : PA12
- LD4 : PD4

System Clock
============

The STM32F769I System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the System clock is driven by the PLL
clock at 216MHz, driven by a 25MHz high speed external clock.

Serial Port
===========

The STM32F769I Discovery kit has up to 8 UARTs. The Zephyr console output is assigned to UART1
which connected to the onboard ST-LINK/V2 Virtual COM port interface. Default communication
settings are 115200 8N1.

Programming and Debugging
*************************

STM32F769I Discovery kit includes an ST-LINK/V2 embedded debug tool interface.

Applications for the ``stm32f769i_disco`` board configuration can be built and
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

Flashing an application to STM32F769I
-------------------------------------------

First, connect the STM32F769I Discovery kit to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32f769i_disco
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
   :board: stm32f769i_disco
   :goals: debug


.. _32F769I-DISCO website:
   https://www.st.com/en/evaluation-tools/32f769idiscovery.html

.. _32F769I-DISCO board User Manual:
   https://www.st.com/resource/en/user_manual/dm00276557.pdf

.. _STM32F769NIH6 on www.st.com:
	https://www.st.com/content/st_com/en/products/microcontrollers/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x9/stm32f769ni.html

.. _STM32F76xxx reference manual:
   https://www.st.com/resource/en/reference_manual/dm00224583.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
