.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: nucleo_h5e5zj

Overview
********

The NUCLEO-H5E5ZJ is an STM32 Nucleo-144 development board based on the
STM32H5E5ZJT6 microcontroller. ST identifies the board as MB2129.

The board provides:

* An Arm® Cortex®-M33 microcontroller running at up to 250 MHz, with an FPU,
  an MPU, and Arm TrustZone® support.
* 4 MiB of internal flash and 1536 KiB of SRAM in an LQFP144 package.
* An embedded ST-LINK debug interface and a USART3 virtual COM port.
* Three user LEDs: green LD1, red LD2, and blue LD3.
* A user push-button and a reset push-button.
* Arduino Uno V3 and ST morpho expansion connectors.
* A USB high-speed interface with an integrated PHY.

See the `NUCLEO-H5E5ZJ product page`_ for board documentation and the
`STM32H5E5ZJ product page`_ for device documentation. The pin assignments below
follow ST's `board configuration`_, `Nucleo BSP`_, and `STM32CubeH5 examples`_.

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Default Peripheral Mapping
==========================

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - Interface
     - Pins
     - Use
   * - USART3 TX / RX
     - PD8 / PD9
     - ST-LINK virtual COM port
   * - UART7 TX / RX
     - PB4 / PA8
     - Arduino D1 / D0
   * - I2C4 SCL / SDA
     - PF5 / PG6
     - Expansion interface
   * - SPI4 SCK / MISO / MOSI
     - PE12 / PE13 / PE14
     - Arduino D13 / D12 / D11
   * - FDCAN1 TX / RX
     - PD5 / PE0
     - External CAN transceiver
   * - LD1, green
     - PA3, active-high
     - ``led0``
   * - LD2, red
     - PF0, active-low
     - ``led1``
   * - LD3, blue
     - PE5, active-low
     - ``led2``
   * - User button
     - PC13, active-high
     - ``sw0``
   * - TIM2 channel 4
     - PA3
     - ``pwm-led0``
   * - ADC1 channels 2 / 6 / 7 / 9 / 3
     - PF11 / PF12 / PA7 / PB0 / PA6
     - Analog inputs
   * - USB OTG HS
     - Dedicated OTG_HS_DM / OTG_HS_DP pins
     - ``zephyr_udc0``

The console uses USART3 at 115200 baud, 8 data bits, no parity, and one stop bit.
UART7 is also available through the ``arduino_serial`` devicetree label.

The green LED is shared by the GPIO and PWM interfaces. Do not use both
interfaces for PA3 at the same time. The PWM configuration uses TIM2 channel 4.

SPI4 configures the three data and clock signals. An application must provide
the chip-select GPIO required by its SPI device.

FDCAN1 is the default ``zephyr,canbus`` controller and requires an external CAN
transceiver.

System Clock
============

The default configuration follows ST's `clock template`_: a 48 MHz external
crystal feeds PLL1, with M = 24, N = 250, and P = 2. The CPU, AHB, and APB
clocks run at 250 MHz. A 32.768 kHz external crystal supplies the RTC.

Memory
======

The board target exposes the full 4 MiB of internal flash. SRAM1 is the default
Zephyr RAM region and provides 256 KiB. SRAM2 through SRAM5 are described by
the SoC devicetree and provide the remaining 1280 KiB; they are not included
in the default linker RAM region.

The target uses TrustZone-disabled operation. It does not define a non-secure
variant or provide a secure firmware image.

Other Interfaces
================

ST's board configuration also describes the following Ethernet RMII signals:

* Reference clock: PA1
* CRS_DV: PD1
* RXD0 / RXD1: PC4 / PC5
* TX_EN: PA5
* TXD0 / TXD1: PB12 / PG12
* MDIO / MDC: PA2 / PC1
* PHY clock output: PD4

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Connect the embedded ST-LINK USB interface to the host. Install
:ref:`STM32CubeProgrammer <stm32cubeprog-flash-host-tools>` to use the default
flash runner.

Building and Flashing
====================

Build and flash the :zephyr:code-sample:`hello_world` application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_h5e5zj
   :goals: build flash

Open the ST-LINK virtual COM port with the console settings above. On Linux,
the port is typically named ``/dev/ttyACM0``. The application prints:

.. code-block:: console

   Hello World! nucleo_h5e5zj/stm32h5e5xx

Debugging
=========

The ST-LINK GDB server is configured as the debug runner:

.. code-block:: console

   west debug --runner stlink_gdbserver

References
**********

.. _NUCLEO-H5E5ZJ product page:
   https://www.st.com/en/evaluation-tools/nucleo-h5e5zj.html

.. _STM32H5E5ZJ product page:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h5e5zj.html

.. _board configuration:
   https://github.com/STMicroelectronics/STM32_open_pin_data/blob/master/boards/G80_Nucleo_NUCLEO-H5E5ZJ_STM32H5E5ZJT6_Board_AllConfig.ioc

.. _Nucleo BSP:
   https://github.com/STMicroelectronics/stm32h5xx-nucleo-bsp

.. _STM32CubeH5 examples:
   https://github.com/STMicroelectronics/STM32CubeH5/tree/main/Projects/NUCLEO-H5E5ZJ

.. _clock template:
   https://github.com/STMicroelectronics/STM32CubeH5/blob/main/Projects/NUCLEO-H5E5ZJ/Templates_Board/Src/main.c
