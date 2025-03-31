.. zephyr:board:: gd32f350r_eval

Overview
********

The GD32F350R-EVAL board is a hardware platform that enables design and debug
of the GigaDevice F350 Cortex-M4F High Performance MCU.

The GD32F350RBT6 features a single-core ARM Cortex-M4F MCU which can run up
to 108-MHz with flash accesses zero wait states, 128kB of Flash, 16kB of
SRAM and 55 GPIOs.

Hardware
********

- GD32F350RBT6 MCU
- AT24C02C 2Kb EEPROM
- 4 x User LEDs
- 4 x User Push buttons
- 1 x USART (RS-232 at J2 connector)
- 1 x POT connected to an ADC input
- Headphone interface
- Micro SD Card Interface
- 2.4'' TFT-LCD (36x48)
- GD-Link on board programmer
- J-Link/SWD connector

For more information about the GD32F350 SoC and GD32F350R-EVAL board:

- `GigaDevice Cortex-M4F Stretch Performance SoC Website`_
- `GD32F350xx Datasheet`_
- `GD32F3x0 User Manual`_
- `GD32F350R-EVAL User Manual`_

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

The GD32F350R-EVAL board has one serial communication port. The default port
is USART0 with TX connected at PA9 and RX at PA10.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Before programming your board make sure to configure boot and serial jumpers as follows:

- J4:  Select 2-3 for both (labeled as ``L``)
- J13: Select 1-2 position (labeled as ``USART``)

Using GD-Link
=============

The GD32F350R-EVAL includes an onboard programmer/debugger (GD-Link) which
allows flash programming and debugging over USB. There is also a SWD header
(J3) which can be used with tools like Segger J-Link.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f350r_eval
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
      :board: gd32f350r_eval
      :goals: flash
      :compact:

   You should see "Hello World! gd32f350r_eval" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32f350r_eval
      :goals: debug
      :compact:

.. _GigaDevice Cortex-M4F Stretch Performance SoC Website:
   https://www.gigadevice.com/products/microcontrollers/gd32/arm-cortex-m4/stretch-performance-line/

.. _GD32F350xx Datasheet:
   http://gd32mcu.com/download/down/document_id/133/path_type/1

.. _GD32F3x0 User Manual:
   http://gd32mcu.com/download/down/document_id/136/path_type/1

.. _GD32F350R-EVAL User Manual:
   https://www.tme.com/Document/ff0a3609934053c07d78ef8662781da9/GD32350R-EVAL%20User%20Manual-V1.0.pdf
