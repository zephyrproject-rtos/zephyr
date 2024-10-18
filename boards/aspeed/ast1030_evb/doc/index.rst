.. zephyr:board:: ast1030_evb

Overview
********

The AST1030_EVB kit is a development platform to evaluate the
Aspeed AST10x0 series SOCs. This board needs to be mated with
part number AST1030.

Hardware
********

- ARM Cortex-M4F Processor
- 768 KB on-chip SRAM for instruction and data memory
- 1 MB on-chip Flash memory for boot ROM and data storage
- SPI interface
- UART interface
- I2C/I3C interface
- FAN PWM interface
- ADC interface
- JTAG interface
- USB interface
- LPC interface
- eSPI interface

Supported Features
==================

The following features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr (at the moment)

The default configuration can be found in
:zephyr_file:`boards/aspeed/ast1030_evb/ast1030_evb_defconfig`


Connections and IOs
===================

Aspeed to provide the schematic for this board.

System Clock
============

The AST1030 SOC is configured to use external 25MHz clock input to generate 200Mhz system clock by
the on-chip PLL.

Serial Port
===========

UART5 is configured for serial logs.  The default serial setup is 115200 8N1.


Programming and Debugging
*************************

This board comes with a JTAG port which facilitates debugging using a single physical connection.

Flashing
========

Build application as usual for the ``ast1030_evb`` board, and flash
using SF100 SPI Flash programmer. See the
`Aspeed Zephyr SDK User Guide`_ for more information.


Debugging
=========

Use JTAG or SWD with a J-Link

References
**********
.. target-notes::

.. _Aspeed Zephyr SDK User Guide:
   https://github.com/AspeedTech-BMC/zephyr/releases/download/v00.01.03/Aspeed_Zephy_SDK_User_Guide_v00.01.03.pdf
