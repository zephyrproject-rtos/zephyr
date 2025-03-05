.. zephyr:board:: gd32a503v_eval

Overview
********

The GD32A503V-EVAL board is a hardware platform that enables design and debug
of the GigaDevice A503 Cortex-M4F High Performance MCU.

The GD32A503VD features a single-core ARM Cortex-M4F MCU which can run up
to 120-MHz with flash accesses zero wait states, 384kiB of Flash, 48kiB of
SRAM and 88 GPIOs.

Hardware
********

- 2 user LEDs
- 2 user push buttons
- Reset Button
- ADC connected to a potentiometer
- 1 DAC channels
- GD25Q16 2Mib SPI Flash
- AT24C02C 2KiB EEPROM
- CS4344 Stereo DAC with Headphone Amplifier
- GD-Link interface

  - CMSIS-DAP swd debug interface over USB HID.

- 2 CAN FD ports

For more information about the GD32A503 SoC and GD32A503V-EVAL board:

- `GigaDevice Cortex-M33 High Performance SoC Website`_
- `GD32A503 Datasheet`_
- `GD32A503 Reference Manual`_
- `GD32A503V Eval Schematics`_
- `GD32 ISP Console`_


Supported Features
==================

.. zephyr:board-supported-hw::

Serial Port
===========

The GD32A503V-EVAL board has 3 serial communication ports. The default port
is UART0 at PIN-72 and PIN-73.

Programming and Debugging
*************************

Before program your board make sure to configure boot setting and serial port.
The default serial port is USART0.

+--------+--------+------------+
| Boot-0 | Boot-1 | Function   |
+========+========+============+
|  1-2   |  1-2   | SRAM       |
+--------+--------+------------+
|  1-2   |  2-3   | Bootloader |
+--------+--------+------------+
|  2-3   |  Any   | Flash      |
+--------+--------+------------+

Using GD-Link
=============

The GD32A503V-EVAL includes an onboard programmer/debugger (GD-Link) which
allow flash programming and debug over USB. There are also program and debug
headers J2 and J100 that can be used with any ARM compatible tools.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32a503v_eval
      :goals: build
      :compact:

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

#. To flash an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32a503v_eval
      :goals: flash
      :compact:

   You should see "Hello World! gd32a503v_eval" in your terminal.

#. To debug an image:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32a503v_eval
      :goals: debug
      :compact:


Using ROM bootloader
====================

The GD32A503 MCU have a ROM bootloader which allow flash programming.  User
should install `GD32 ISP Console`_ software at some Linux path.  The recommended
is :code:`$HOME/.local/bin`.

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: gd32a503v_eval
      :goals: build
      :compact:

#. Enable board bootloader:

   - Remove boot-0 jumper
   - press reset button

#. To flash an image:

   .. code-block:: console

      west flash -r gd32isp [--port=/dev/ttyUSB0]

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Press reset button

   You should see "Hello World! gd32a503v_eval" in your terminal.


.. _GigaDevice Cortex-M33 High Performance SoC Website:
	https://www.gigadevice.com.cn/product/mcu/arm-cortex-m33/gd32a503vdt3

.. _GD32A503 Datasheet:
	https://www.gd32mcu.com/download/down/document_id/401/path_type/1

.. _GD32A503 Reference Manual:
	https://www.gd32mcu.com/download/down/document_id/402/path_type/1

.. _GD32A503V Eval Schematics:
	https://www.gd32mcu.com/download/down/document_id/404/path_type/1

.. _GD32 ISP Console:
	http://www.gd32mcu.com/download/down/document_id/175/path_type/1
