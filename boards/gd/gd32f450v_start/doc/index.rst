.. zephyr:board:: gd32f450v_start

Overview
********

The GD32F450V-START board is a hardware platform that enables prototyping
on GD32F450VK Cortex-M4F Stretch Performance MCU.

The GD32F450VK features a single-core ARM Cortex-M4F MCU which can run up
to 200 MHz with flash accesses zero wait states, 3072kiB of Flash, 256kiB of
SRAM and 82 GPIOs.

Hardware
********

- GD32F450VKT6 MCU
- 1 x User LEDs
- 1 x User Push buttons
- USB FS/HS connectors
- GD-Link on board programmer
- J-Link/SWD connector

For more information about the GD32F450 SoC and GD32F450V-START board:

- `GigaDevice Cortex-M4F Stretch Performance SoC Website`_
- `GD32F450X Datasheet`_
- `GD32F4XX User Manual`_
- `GD32F450V-START User Manual`_

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
     - :dtcompatible:`arm,v8m-nvic`
   * - PWM
     - :kconfig:option:`CONFIG_PWM`
     - :dtcompatible:`gd,gd32-pwm`
   * - SYSTICK
     - N/A
     - N/A
   * - USART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`gd,gd32-usart`

Other peripherals may be used if shields are connected to the board.

Serial Port
===========

The GD32F450V-START board has no exposed serial communication port. The board
provides default configuration for USART0 with TX connected at PB6 and RX at
PB7. PB6/PB7 are exposed in JP6, so you can solder a connector and use a
UART-USB adapter.

Programming and Debugging
*************************

Before programming your board make sure to configure boot jumpers as
follows:

- JP2/3: Select 2-3 for both (boot from user memory)

Using GD-Link
=============

The GD32F450V-START includes an onboard programmer/debugger (GD-Link) which
allows flash programming and debugging over USB. There is also a SWD header
(JP100) which can be used with tools like Segger J-Link.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f450v_start
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
      :board: gd32f450v_start
      :goals: flash
      :compact:

   You should see "Hello World! gd32f450v_start" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f450v_start
      :goals: debug
      :compact:

.. _GigaDevice Cortex-M4F Stretch Performance SoC Website:
   https://www.gigadevice.com/products/microcontrollers/gd32/arm-cortex-m4/stretch-performance-line/

.. _GD32F450X Datasheet:
   https://gd32mcu.com/data/documents/datasheet/GD32F450xx_Datasheet_Rev2.3.pdf

.. _GD32F4xx User Manual:
   https://www.gigadevice.com/manual/gd32f450xxxx-user-manual/

.. _GD32F450V-START User Manual:
   https://gd32mcu.com/data/documents/evaluationBoard/GD32F4xx_Demo_Suites_V2.6.1.rar
