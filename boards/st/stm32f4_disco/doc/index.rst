.. _stm32f4_disco_board:

ST STM32F4 Discovery
####################

Overview
********

The STM32F4DISCOVERY Discovery kit features an ARM Cortex-M4 based STM32F407VG MCU
with a wide range of connectivity support and configurations Here are
some highlights of the STM32F4DISCOVERY board:


- STM32 microcontroller in LQFP100 package
- Extension header for all LQFP100 I/Os for quick connection to prototyping board and easy probing
- On-board ST-LINK/V2 debugger/programmer with SWD connector
- Flexible board power supply:

       - USB VBUS or external source(3.3V, 5V, 7 - 12V)
       - Power management access point

- Eight LEDs:

       - USB communication (LD1)
       - 3.3 V power on (LD2)
       - Four user LEDs: orange (LD3), green (LD4), red (LD5), and blue (LD6)
       - 2 USB OTG LEDs for VBUS (LD7) and over-current (LD8)

- Two push-buttons: USER and RESET
- USB OTG FS with micro-AB connector
- LIS302DL or LIS3DSH ST MEMS 3-axis accelerometer
- MP45DT02 ST-MEMS audio sensor omni-directional digital microphone
- CS43L22 audio DAC with integrated class D speaker driver

.. image:: img/stm32f4_disco.jpg
     :align: center
     :alt: STM32F4DISCOVERY

More information about the board can be found at the `STM32F4DISCOVERY website`_.

Hardware
********

STM32F4DISCOVERY Discovery kit provides the following hardware components:

- STM32F407VGT6 in LQFP100 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 168 MHz max CPU frequency
- VDD from 1.8 V to 3.6 V
- 1 MB Flash
- 192+4 KB SRAM including 64-Kbyte of core coupled memory
- GPIO with external interrupt capability
- 3x12-bit ADC with 24 channels
- 2x12-bit D/A converters
- RTC
- Advanced-control Timer
- General Purpose Timers (17)
- Watchdog Timers (2)
- USART/UART (6)
- I2C (3)
- SPI (3)
- SDIO
- 2xCAN
- USB 2.0 OTG FS with on-chip PHY
- USB 2.0 OTG HS/FS with dedicated DMA, on-chip full-speed PHY and ULPI
- 10/100 Ethernet MAC with dedicated DMA
- 8- to 14-bit parallel camera
- CRC calculation unit
- True random number generator
- DMA Controller

More information about STM32F407VG can be found here:
       - `STM32F407VG on www.st.com`_
       - `STM32F407 reference manual`_

Supported Features
==================

The Zephyr stm32f4_disco board configuration supports the following hardware features:

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
| USB       | on-chip    | usb                                 |
+-----------+------------+-------------------------------------+
| CAN       | on-chip    | CAN controller                      |
+-----------+------------+-------------------------------------+

.. note:: CAN feature requires CAN transceiver, such as `SK Pang CAN breakout board`_.
          Zephyr default configuration uses CAN_2 exclusively, as simultaneous use
          of CAN_1 and CAN_2 is not yet supported.

Other hardware features are not yet supported on Zephyr porting.

The default configuration can be found in
:zephyr_file:`boards/st/stm32f4_disco/stm32f4_disco_defconfig`


Pin Mapping
===========

STM32F4DISCOVERY Discovery kit has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

For more details please refer to `STM32F4DISCOVERY board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART_1_TX : PB6
- UART_1_RX : PB7
- UART_2_TX : PA2
- UART_2_RX : PA3
- USER_PB : PA0
- LD3 : PD13
- LD4 : PD12
- LD5 : PD14
- LD6 : PD15
- USB DM : PA11
- USB DP : PA12
- CAN1_RX : PB8
- CAN1_TX : PB9
- CAN2_RX : PB5
- CAN2_TX : PB13

System Clock
============

STM32F4DISCOVERY System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at 168MHz,
driven by 8MHz high speed external clock.

Serial Port
===========

STM32F4DISCOVERY Discovery kit has up to 6 UARTs. The Zephyr console output is assigned to UART2.
Default settings are 115200 8N1.
Please note that ST-Link Virtual Com Port is not wired to chip serial port. In order to
enable console output you should use a serial cable and connect it to UART2 pins (PA2/PA3).


Programming and Debugging
*************************

STM32F4DISCOVERY Discovery kit includes an ST-LINK/V2 embedded debug tool interface.

Applications for the ``stm32f4_disco`` board configuration can be built and
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

Flashing an application to STM32F4DISCOVERY
-------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32f4_disco
   :goals: build flash

You should see user led "LD4" blinking.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32f4_disco
   :maybe-skip-config:
   :goals: debug

.. _STM32F4DISCOVERY website:
   https://www.st.com/en/evaluation-tools/stm32f4discovery.html

.. _STM32F4DISCOVERY board User Manual:
   https://www.st.com/resource/en/user_manual/dm00039084.pdf

.. _STM32F407VG on www.st.com:
   https://www.st.com/en/microcontrollers/stm32f407vg.html

.. _STM32F407 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00031020.pdf

.. _SK Pang CAN breakout board:
   https://www.skpang.co.uk/products/can-bus-can-fd-breakout-board-5v-supply-and-3-3v-logic

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
