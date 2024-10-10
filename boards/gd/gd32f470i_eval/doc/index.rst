.. _gd32f470i_eval:

GigaDevice GD32F470I-EVAL
#########################

Overview
********

The GD32F470I-EVAL board is a hardware platform that enables prototyping
on GD32F470IK Cortex-M4F Stretch Performance MCU.

The GD32F470IK features a single-core ARM Cortex-M4F MCU which can run up
to 240 MHz with flash accesses zero wait states, 3072kiB of Flash, 256kiB of
SRAM and 140 GPIOs.

.. image:: img/gd32f470i_eval.jpg
     :align: center
     :alt: gd32f470i_eval


Hardware
********

- GD32F470IKH6 MCU
- 2Kb EEPROM
- 16Mbit SPI and QSPI NOR Flash
- 256Mbit SDRAM
- 3 x User LEDs
- 3 x User Push buttons
- 1 x USART (RS-232 at J1 connector)
- 1 x POT connected to an ADC input
- Headphone interface
- Micro SD Card Interface
- USB FS connector
- USB HS connector
- 1 x CAN
- Ethernet Interface
- 4.3" LCD (480x272)
- OV2640 Digital Camera
- GD-Link on board programmer
- J-Link/JTAG connector

For more information about the GD32F470 SoC and GD32F470I-EVAL board:

- `GigaDevice Cortex-M4F Stretch Performance SoC Website`_
- `GD32F470IKH6 Specifications`_
- `GD32F470xx Datasheet`_
- `GD32F4xx User Manual`_

Supported Features
==================

The board configuration supports the following hardware features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - EXTI
     - :kconfig:option:`CONFIG_GD32_EXTI`
     - :dtcompatible:`gd,gd32-exti`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`gd,gd32-gpio`
   * - NVIC
     - N/A
     - :dtcompatible:`arm,v7m-nvic`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`gd,gd32-pwm`
   * - SYSTICK
     - N/A
     - N/A
   * - USART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`gd,gd32-usart`
   * - DAC
     - :kconfig:option:`CONFIG_DAC`
     - :dtcompatible:`gd,gd32-dac`
   * - I2C
     - :kconfig:option:`CONFIG_I2C`
     - :dtcompatible:`gd,gd32-i2c`
   * - EEPROM
     - :kconfig:option:`CONFIG_EEPROM`
     - :dtcompatible:`atmel,at24`
   * - SPI
     - :kconfig:option:`CONFIG_SPI`
     - :dtcompatible:`gd,gd32-spi`

Serial Port
===========

The GD32F470I-EVAL board has one serial communication port. The default port
is USART0 with TX connected at PA9 and RX at PA10.

Programming and Debugging
*************************

Before programming your board make sure to configure boot and serial jumpers
as follows:

- J2/3: Select 2-3 for both (boot from user memory)
- J5: Select 1-2 position (labeled as ``USART0``)

Using GD-Link
=============

The GD32F470I-EVAL includes an onboard programmer/debugger (GD-Link) which
allows flash programming and debugging over USB. There is also a JTAG header
(J1) which can be used with tools like Segger J-Link.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f470i_eval
      :goals: build
      :compact:

#. Run your favorite terminal program to listen for output. On Linux the
   terminal should be something like ``/dev/ttyUSB0``. For example:

   .. code-block:: console

      minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f470i_eval
      :goals: flash
      :compact:

   You should see "Hello World! gd32f470i_eval" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f470i_eval
      :goals: debug
      :compact:


.. _GigaDevice Cortex-M4F Stretch Performance SoC Website:
   https://www.gigadevice.com/products/microcontrollers/gd32/arm-cortex-m4/stretch-performance-line/gd32f470-series/

.. _GD32F470IKH6 Specifications:
   https://www.gigadevice.com/microcontroller/gd32f470ikh6/

.. _GD32F470xx Datasheet:
   https://gd32mcu.com/data/documents/datasheet/GD32F470xx_Datasheet_Rev1.3.pdf

.. _GD32F4xx User Manual:
   https://gd32mcu.com/data/documents/userManual/GD32F4xx_User_Manual_Rev2.7.pdf
