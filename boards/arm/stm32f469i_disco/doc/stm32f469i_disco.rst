.. _stm32f469i_disco_board:

ST STM32F469I Discovery
#######################

Overview
********

The STM32F469 Discovery kit features an ARM Cortex-M4 based STM32F469NI MCU
with a wide range of connectivity support and configurations Here are
some highlights of the STM32F469I-DISCO board:


- STM32 microcontroller in BGA216 package
- On-board ST-LINK/V2-1 debugger/programmer, supporting USB reenumeration capability
- Flexible board power supply:

       - ST-LINK/V2-1 USB connector
       - User USB FS connector
       - VIN from Arduino* compatible connectors

- Four user LEDs
- Two push-buttons: USER and RESET
- USB OTG FS with micro-AB connector
- 4-inch 800x480 pixel TFT color LCD with MIPI DSI interface and capacitive touch screen
- SAI Audio DAC, with a stereo headphone output jack
- Three MEMS microphones
- MicroSD card connector
- I2C extension connector
- 4Mx32bit SDRAM
- 128-Mbit Quad-SPI NOR Flash
- Expansion connectors and Arduino UNO V3 connectors

.. image:: img/en.stm32f469i-disco.jpg
     :width: 457px
     :align: center
     :height: 551px
     :alt: STM32F469I-DISCO

More information about the board can be found at the `32F469IDISCOVERY website`_.

Hardware
********

STM32F469I-DISCO Discovery kit provides the following hardware components:

- STM32F469NIH6 in BGA216 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 180 MHz max CPU frequency
- VDD from 1.8 V to 3.6 V
- 2 MB Flash
- 384+4 KB SRAM including 64-Kbyte of core coupled memory
- GPIO with external interrupt capability
- LCD parallel interface, 8080/6800 modes
- LCD TFT controller supporting up to XGA resolution
- MIPI |reg|  DSI host controller supporting up to 720p 30Hz resolution
- 3x12-bit ADC with 24 channels
- 2x12-bit D/A converters
- RTC
- Advanced-control Timer
- General Purpose Timers (17)
- Watchdog Timers (2)
- USART/UART (8)
- I2C (3)
- SPI (6)
- 1xSAI (serial audio interface)
- SDIO
- 2xCAN
- USB 2.0 OTG FS with on-chip PHY
- USB 2.0 OTG HS/FS with dedicated DMA, on-chip full-speed PHY and ULPI
- 10/100 Ethernet MAC with dedicated DMA
- 8- to 14-bit parallel camera
- CRC calculation unit
- True random number generator
- DMA Controller

More information about STM32F469NI can be found here:
       - `STM32F469NI on www.st.com`_
       - `STM32F469 reference manual`_

Supported Features
==================

The Zephyr stm32f469i_disco board configuration supports the following hardware features:

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
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on Zephyr porting.

The default configuration can be found in the defconfig file:

	``boards/arm/stm32f469i_disco/stm32f469i_disco_defconfig``


Pin Mapping
===========

STM32F469I-DISCO Discovery kit has 9 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For mode details please refer to `32F469IDISCOVERY board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------
- UART_3_TX : PB10
- UART_3_RX : PB11
- UART_6_TX : PG14
- UART_6_RX : PG9
- USER_PB : PA0
- LD1 : PG6
- LD2 : PD4
- LD3 : PD5
- LD4 : PK3

System Clock
============

STM32F469I-DISCO System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at 180MHz,
driven by 8MHz high speed external clock.

Serial Port
===========

The STM32F469 Discovery kit has up to 8 UARTs. The Zephyr console output is assigned to UART3.
Default settings are 115200 8N1.


Programming and Debugging
*************************

Flashing
========

STM32F469I-DISCO Discovery kit includes an ST-LINK/V2 embedded debug tool interface.
This interface is supported by the openocd version included in Zephyr SDK.

Flashing an application to STM32F469I-DISCO
-------------------------------------------

The sample application :ref:`hello_world` is being used in this tutorial:

.. code-block:: console

   $<zephyr_root_path>/samples/hello_world

To build the Zephyr kernel and application, enter:

.. code-block:: console

   $ cd <zephyr_root_path>
   $ source zephyr-env.sh
   $ cd $ZEPHYR_BASE/samples/hello_world/
   $ make BOARD=stm32f469i_disco

Connect the STM32F469I-DISCO Discovery kit to your host computer using the USB port.
Then, enter the following command:

.. code-block:: console

   $ make BOARD=stm32f469i_disco flash

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message:

.. code-block:: console

   Hello World! arm


Debugging
=========

Access gdb with the following make command:

.. code-block:: console

   $ make BOARD=stm32f469i_disco debug


.. _32F469IDISCOVERY website:
   http://www.st.com/en/evaluation-tools/32f469idiscovery.html

.. _32F469IDISCOVERY board User Manual:
   http://www.st.com/resource/en/user_manual/dm00218846.pdf

.. _STM32F469NI on www.st.com:
   http://www.st.com/en/microcontrollers/stm32f469ni.html

.. _STM32F469 reference manual:
   http://www.st.com/resource/en/reference_manual/dm00127514.pdf
