.. _elan_32f967_dv:

Elan 32f967_dv
##############

Overview
********

The Elan 32f967_dv is a development board based on the Elan em32f967
SoC (ARM Cortex-M4). This board is used to validate the initial SoC
integration with Zephyr.

Supported Features
==================

The following features are currently supported:

* Build with `samples/hello_world`

Not yet supported:
* UART, GPIO, and other peripherals

Programming and Debugging
*************************

This board does not use a standard flashing interface such as J-Link or OpenOCD.

Firmware programming is performed using Elan's proprietary flashing tool,
which communicates with the device over USB and requires specific Backup
Register and Watchdog Timer configurations to enter the bootloader mode.

The flashing tool is distributed only to Elan's customers for production and
evaluation purposes. It is not publicly available.

At this stage, the Zephyr `west flash` command is not supported.
Users can build the firmware using:

.. code-block:: console

   west build -p always -b 32f967_dv samples/hello_world

and then flash the generated `zephyr.bin` using Elan's internal flashing tool.
