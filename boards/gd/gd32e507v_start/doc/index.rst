.. zephyr:board:: gd32e507v_start

Overview
********

The GD32E507V-START board is a hardware platform that enables prototyping
on GD32E507VE Cortex-M33 High Performance MCU.

The GD32E507VE features a single-core ARM Cortex-M33 MCU which can run up
to 180 MHz with flash accesses zero wait states, 512kiB of Flash, 128kiB of
SRAM and 80 GPIOs.

Hardware
********

- GD32E507VET6 MCU
- 1 x User LEDs
- 1 x User Push buttons
- 1 x USART (RS-232 at J1 connector)
- GD-Link on board programmer
- J-Link/SWD connector

For more information about the GD32E507 SoC and GD32E507V-START board:

- `GigaDevice Cortex-M33 High Performance SoC Website`_
- `GD32E507X Datasheet`_
- `GD32E50X User Manual`_
- `GD32E507V-START User Manual`_

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

The GD32E507V-START board has one serial communication port. The default port
is USART0 with TX connected at PB6 and RX at PB7. USART0 is exposed as a
virtual COM port via the CN3 USB connector.

Programming and Debugging
*************************

Before programming your board make sure to configure boot jumpers as
follows:

- JP3/4: Select 2-3 for both (boot from user memory)

Using GD-Link or J-Link
=======================

The board comes with an embedded GD-Link programmer. It can be used with pyOCD
provided you install the necessary CMSIS-Pack:

.. code-block:: console

   pyocd pack install gd32e507ve

J-Link can also be used to program the board using the SWD interface exposed in
the JP1 header.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32e507v_start
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
      :board: gd32e507v_start
      :goals: flash
      :compact:

   You should see "Hello World! gd32e507v_start" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32e507v_start
      :goals: debug
      :compact:

.. _GigaDevice Cortex-M33 High Performance SoC Website:
   https://www.gigadevice.com/products/microcontrollers/gd32/arm-cortex-m33/high-performance-line/

.. _GD32E507X Datasheet:
   https://gd32mcu.com/download/down/document_id/252/path_type/1

.. _GD32E50X User Manual:
   https://www.gd32mcu.com/download/down/document_id/249/path_type/1

.. _GD32E507V-START User Manual:
   https://www.gd32mcu.com/data/documents/evaluationBoard/GD32E50x_Demo_Suites_V1.2.1.rar
