.. _ks_zion_board:

Kernelspace Zion
################

Overview
********

The Kernelspace Zion is a small generic development board, featuring an ARM Cortex-M4 STM32F303K8
MCU with the minal required hardware layout for beginners to get started quickly. It's a simple
open-hardware board that has been thought to be assembled easily at home.

Some highlights:

- STM32 microcontroller in LQFP32 package
- GPIO pads all around (JXX) for a generic/quick usage
- I2C display
- Board power supply: through USB connector, 5 V supply voltage
- Reset push button
- One generic LEDs
- JTAG header
- Optional external crystal

.. image:: img/ks_zion.webp
   :align: center
   :alt: Zion

More details, schematic and pcb layout are available at `Kernelspace Zion website`_


Hardware
********

- STM32F303K8 in LQFP32 package
- ARM |reg| 32-bit Cortex |reg| -M4 CPU with FPU
- 72 MHz max CPU frequency
- VDD 5.0V
- 64 KB Flash
- 12 KB SRAM
- GPIO with external interrupt capability
- 12-bit ADC with 21 channels
- 12-bit D/A converter
- DMA channels (7)
- 22 PWM channels
- SPI (1)
- I2C (1)
- USART (3)
- CAN (1)
- RTC
- General Purpose Timers (5)
- Infrared transmitter
- I2C display

Supported Features
==================

The Zephyr Zion board configuration supports the following hardware features:

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
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | ADC Controller                      |
+-----------+------------+-------------------------------------+

Default peripheral mapping
==========================
.. rst-class:: rst-columns

- XTAL : PF1
- XTAL : PF0
- J25 : PB3 : JTDO
- J3 : PB0
- J4 : PB1
- J5 : PB4
- J6 : PB5
- J7 : PB6 : I2C_1 SCL
- J8 : PB7 : I2C_1 SDA
- J9 : PA0
- J10 : PA1
- J11 : PA2
- J12 : PA3
- J13 : PA4
- J14 : PA5
- J15 : PA6
- MOSFET : PA7
- J16 : PA8
- J17 : PA9
- J18 : PA10
- J19 : PA11
- LED : PA12
- J25 : PA13 : JTMS
- J25 : PA14 : JTCK
- J25 : PA15 : JTDI

System Clock
============

Oscillator is set by default as hsi, 8Mhz, and system clock set to 68Mhz.


Programming and Debugging
*************************

Applications for the ``ks_zoion`` board configuration can be built and flashed in the
usual way (see :ref:`build_an_application` and :ref:`application_run` for more details).

Flashing
========

The board can be programmed by the ST-LINK/V2-1 debug tool interface.
This interface is supported by the openocd version included in the Zephyr SDK.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: ks_zion
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! arm

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/blinky
   :board: ks_zion
   :goals: debug

.. _Kernelspace Zion website:
   https://kernel-space.org/hardware/zion.html
