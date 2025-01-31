.. zephyr:board:: stm32f413h_disco

Overview
********

The STM32F413H-DISCO Discovery kit features an ARM Cortex-M4 based STM32F413ZH MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the STM32F413H-DISCO board:


- STM32F413ZHT6 microcontroller featuring 1.5 Mbyte of Flash memory and 320 Kbytes of RAM in an LQFP144 package
- On-board ST-LINK/V2-1 SWD debugger supporting USB re-enumeration capability:

       - USB virtual COM port
       - mass storage
       - debug port

- 1.54 inch 240x240 pixel TFT color LCD with parallel interface and capacitive touchscreen
- I2S Audio CODEC, with a stereo headset jack, including analog microphone input and a loudspeaker output
- Stereo digital MEMS microphones
- MicroSD card connector extension
- I2C extension connector
- 128 Mbit Quad-SPI Nor Flash
- 8 Mbit 16-bit wide PSRAM
- Reset and User buttons
- Two color user LEDs.
- USB OTG FS with Micro-AB connector
- Four power supply options:

       - ST-LINK/V2-1 USB connector
       - User USB FS connector
       - VIN from Arduino* connectors
       - + 5 V from Arduino* connectors

- Two power supplies for MCU: 2.0 V and 3.3 V
- Compatible with Arduino(tm) Uno revision 3 connectors
- Extension connector for direct access to various features of STM32F413ZHT6 MCU
- Comprehensive free software including a variety of examples, part of STM32Cube package

More information about the board can be found at the `32F413HDISCOVERY website`_.

Hardware
********

STM32F413H-DISCO Discovery kit provides the following hardware components:

- STM32F413ZHT6 in LQFP144 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 100 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 1.5 MB Flash
- 320 KB SRAM
- GPIO with external interrupt capability
- LCD parallel interface, 8080/6800 modes
- 1x12-bit ADC with 16 channels
- RTC
- Advanced-control Timer
- General Purpose Timers (12)
- Watchdog Timers (2)
- USART/UART (10)
- I2C (4)
- SPI (5)
- SDIO
- SAI
- 3xCAN
- USB OTG 2.0 Full-speed
- CRC calculation unit
- True random number generator
- DMA Controller

More information about STM32F413ZH can be found here:
       - `STM32F413ZH on www.st.com`_
       - `STM32F413 reference manual`_

Supported Features
==================

The Zephyr STM32F413H-DISCO board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported by Zephyr.

The default configuration can be found in
:zephyr_file:`boards/st/stm32f413h_disco/stm32f413h_disco_defconfig`


Pin Mapping
===========

STM32F413H-DISCO Discovery kit has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `32F413HDISCOVERY board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------
- UART_6_TX : PG14
- UART_6_RX : PG9
- LD1 : PE3
- LD2 : PC5

System Clock
============

STM32F413H-DISCO System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at 100MHz,
that is driven by the internal oscillator.

Serial Port
===========

The STM32F413H-DISCO Discovery kit has up to 10 UARTs. The Zephyr console output is assigned to UART6.
Default settings are 115200 8N1.


Programming and Debugging
*************************

STM32F413H-DISCO Discovery kit includes an ST-LINK/V2 embedded debug tool interface.

Applications for the STM32F413H-DISCO board configuration can be built and
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

Flashing an application to STM32F413H-DISCO
-------------------------------------------

Connect the STM32F413H-DISCO Discovery kit to your host computer using
the USB port, then run a serial host program to connect with your
board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash an application. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32f413h_disco
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32f413h_disco/stm32f413xx


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32f413h_disco
   :maybe-skip-config:
   :goals: debug

.. _32F413HDISCOVERY website:
   https://www.st.com/en/evaluation-tools/32f413hdiscovery.html

.. _32F413HDISCOVERY board User Manual:
   https://www.st.com/resource/en/user_manual/um2135-discovery-kit-with-stm32f413zh-mcu-stmicroelectronics.pdf

.. _STM32F413ZH on www.st.com:
   https://www.st.com/en/microcontrollers/stm32f413zh.html

.. _STM32F413 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0430-stm32f413423-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
