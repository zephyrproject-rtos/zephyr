.. _ch569w_evt:

CH569W-EVT
##########

Overview
********

The CH569W-EVT is a evaluation board for the CH569 chip.

CH569/9 is a member of the QingKe V3A family developed by WCH.

Hardware
********
- QingKe V3A RISC-V RV32IMAC core, running up to 120MHz
- 448KB Flash
- 32/64/96KB + 16KB SRAM
- 49 GPIOs
- 4 UARTs
- 2 SPIs
- 1 HSPI
- USB 2.0 High Speed
- USB 3.0 Super Speed

Supported Features
==================
The CH569W-EVT supports the following features:

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - Clock Mux
     - N/A
     - :dtcompatible:`wch,ch56x-clkmux`
   * - GPIO
     - :kconfig:option:`CONFIG_GPIO`
     - :dtcompatible:`wch,ch56x-gpio`
   * - PFIC
     - N/A
     - :dtcompatible:`wch,ch32v-pfic`
   * - PINCTRL
     - :kconfig:option:`CONFIG_PINCTRL`
     - :dtcompatible:`wch,ch5xx-pinctrl`
   * - SYSTICK
     - N/A
     - :dtcompatible:`wch,ch32v-v3-systick`
   * - UART
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`wch,ch5xx-uart`

Other hardware features have not been enabled yet for this board.

Programming and Debugging
*************************

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: ch569w_evt
   :goals: build

Flashing
========

The WCH MCUs can be flashed via USB (using WCHISPTool), or via WCH-Link.

WCHISPTool
----------

`WCHISPTool_CMD`_ is required to flash the board. Add it to the ``PATH``
environment variable, and refer to the tool's documentation for generating the
device configuration file.

.. code-block:: sh

	west flash --device XXXX --cfg-file /path/to/CH569.ini

.. _WCHISPTool_CMD:
   https://wch-ic.com/downloads/WCHISPTool_CMD_ZIP.html
