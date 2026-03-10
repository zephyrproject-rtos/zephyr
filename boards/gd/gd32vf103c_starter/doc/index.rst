.. zephyr:board:: gd32vf103c_starter

Overview
********

The GD32VF103C-STARTER board is a hardware platform that enables prototyping
on GD32VF103CB RISC-V MCU.

The GD32VF103CB features a single-core RISC-V 32-bit MCU which can run up
to 108 MHz with flash accesses zero wait states, 128 KiB of Flash, 32 KiB of
SRAM and 37 GPIOs.

Hardware
********

- GD32VF103CBT6 MCU
- 1 x User LEDs
- 1 x USART (USB port with CH340E)
- USB FS connector
- GD-Link on board programmer
- J-Link/JTAG connector

For more information about the GD32VF103 SoC and GD32VF103C-STARTER board:

- `GigaDevice RISC-V Mainstream SoC Website`_
- `GD32VF103 Datasheet`_
- `GD32VF103 User Manual`_
- `GD32VF103C-STARTER Documents`_

Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

The GD32VF103C-STARTER board has one serial communications port.
TX connected at PA9 and RX at PA10.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Before programming your board make sure to configure boot and serial jumpers
as follows:

- JP2/3: Select 2-3 for both (boot from user memory)
- JP5/6: Select 1-2 positions (labeled as ``USART0``)

Using GD-Link
=============

The GD32VF103C-STARTER includes an onboard programmer/debugger (GD-Link) which
allows flash programming and debugging over USB. There is also a JTAG header
(JP1) which can be used with tools like Segger J-Link.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32vf103c_starter
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
      :board: gd32vf103c_starter
      :goals: flash
      :compact:

   You should see "Hello World! gd32vf103c_starter" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32vf103c_starter
      :goals: debug
      :compact:


.. _GigaDevice RISC-V Mainstream SoC Website:
   https://www.gigadevice.com/products/microcontrollers/gd32/risc-v/mainstream-line/

.. _GD32VF103 Datasheet:
   https://www.gigadevice.com/datasheet/gd32vf103xxxx-datasheet/

.. _GD32VF103 User Manual:
   http://www.gd32mcu.com/download/down/document_id/222/path_type/1

.. _GD32VF103C-STARTER Documents:
   https://github.com/riscv-mcu/GD32VF103_Demo_Suites/tree/master/GD32VF103C_START_Demo_Suites/Docs
