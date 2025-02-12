.. _nucleo_f756zg_board:

ST Nucleo F756ZG
################

Overview
********

The STM32 Nucleo-144 boards offer combinations of performance and power that
provide an affordable and flexible way for users to build prototypes and try
out new concepts. For compatible boards, the SMPS significantly reduces power
consumption in Run mode.

The Arduino-compatible ST Zio connector expands functionality of the Nucleo
open development platform, with a wide choice of specialized Arduino* Uno V3
shields.

The STM32 Nucleo-144 board does not require any separate probe as it integrates
the ST-LINK/V2-1 debugger/programmer.

The STM32 Nucleo-144 board comes with the STM32 comprehensive free software
libraries and examples available with the STM32Cube MCU Package.

Key Features

- STM32 microcontroller in LQFP144 package
- Ethernet compliant with IEEE-802.3-2002 (depending on STM32 support)
- USB OTG or full-speed device (depending on STM32 support)
- 3 user LEDs
- 2 user and reset push-buttons
- 32.768 kHz crystal oscillator
- Board connectors:

  - USB with Micro-AB
  - SWD
  - Ethernet RJ45 (depending on STM32 support)
  - ST Zio connector including Arduino* Uno V3
  - ST morpho

- Flexible power-supply options: ST-LINK USB VBUS or external sources.
- On-board ST-LINK/V2-1 debugger/programmer with USB re-enumeration
  capability: mass storage, virtual COM port and debug port.
- Comprehensive free software libraries and examples available with the
- STM32Cube MCU package.
- Arm* Mbed Enabled* compliant (only for some Nucleo part numbers)

.. image:: img/nucleo_f756zg.jpg
   :align: center
   :alt: Nucleo F756ZG

More information about the board can be found at the `Nucleo F756ZG website`_.

Hardware
********

Nucleo F756ZG provides the following hardware components:

- STM32F756ZG in LQFP144 package
- ARM 32-bit Cortex-M7 CPU with FPU
- Chrom-ART Accelerator
- ART Accelerator
- 216 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 1 MB Flash
- 320 KB SRAM
- 16-bit timers(10)
- 32-bit timers(2)
- SPI(6)
- I2C(4)
- I2S (3)
- USART(4)
- UART(4)
- USB OTG Full Speed and High Speed(1)
- USB OTG Full Speed(1)
- CAN(2)
- SAI(2)
- SPDIF_Rx(4)
- HDMI_CEC(1)
- Dual Mode Quad SPI(1)
- Camera Interface
- GPIO(up to 168) with external interrupt capability
- 12-bit ADC(3) with 24 channels / 2.4 MSPS
- 12-bit DAC with 2 channels(2)
- True Random Number Generator (RNG)
- 16-channel DMA
- LCD-TFT Controller with XGA resolution

Supported Features
==================

The Zephyr nucleo_f756zg board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| ETHERNET  | on-chip    | ethernet                            |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | usb_device                          |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in
:zephyr_file:`boards/st/nucleo_f756zg/nucleo_f756zg_defconfig`

For more details please refer to `STM32 Nucleo-144 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

The Nucleo F756ZG board features a ST Zio connector (extended Arduino Uno V3)
and a ST morpho connector. Board is configured as follows:

- UART_2 TX/RX/RTS/CTS : PD5/PD6/PD4/PD3
- UART_3 TX/RX : PD8/PD9 (ST-Link Virtual Port Com)
- UART_6 TX/RX : PG14/PG9 (Arduino UART)
- USER_PB : PC13
- LD1 : PB0
- LD2 : PB7
- LD3 : PB14
- ETH : PA1, PA2, PA7, PB13, PC1, PC4, PC5, PG11, PG13
- USB DM : PA11
- USB DP : PA12
- I2C : PB8, PB9
- PWM : PE13
- SPI : PD14, PA5, PA6, PA7

Note. The Arduino Uno v3 specified SPI device conflicts with the on-board ETH
device on pin PA7.

System Clock
------------

Nucleo F756ZG System Clock could be driven by an internal or external
oscillator, as well as the main PLL clock. By default, the System clock is
driven by the PLL clock at 72MHz, driven by an 8MHz high-speed external clock.

Serial Port
-----------

Nucleo F756ZG board has 4 UARTs and 4 USARTs. The Zephyr console output is
assigned to UART3. Default settings are 115200 8N1.


Programming and Debugging
*************************

Applications for the ``nucleo_f756zg`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Nucleo F756ZG board includes an ST-LINK/V2-1 embedded debug tool interface.

Flashing an application to Nucleo F756ZG
----------------------------------------

Here is an example for the :ref:`hello_world` application.

Run a serial host program to connect with your Nucleo board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f756zg
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! nucleo_f756zg

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f756zg
   :maybe-skip-config:
   :goals: debug

.. _Nucleo F756ZG website:
   https://www.st.com/en/evaluation-tools/nucleo-f756zg.html

.. _STM32 Nucleo-144 board User Manual:
   https://www.st.com/resource/en/user_manual/dm00105823.pdf

.. _STM32F756ZG on www.st.com:
   https://www.st.com/content/st_com/en/products/microcontrollers/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32f7-series/stm32f7x6/stm32f756zg.html

.. _STM32F756 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00124865.pdf
