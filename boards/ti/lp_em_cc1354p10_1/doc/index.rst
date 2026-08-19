.. zephyr:board:: lp_em_cc1354p10_1

Overview
********

The Texas Instruments CC1354P10_1 LaunchPad™ (LP_EM_CC1354P10_1) is a development
kit for the SimpleLink™ multi-standard CC1354P10 wireless MCU.

See the `TI CC1354P10_1 LaunchPad Product Page`_ for details.

Hardware
********

The CC1354P10_1 LaunchPad™ development kit features the CC1354P10 wireless MCU.
The board is equipped with two LEDs, two push buttons and BoosterPack connectors
for expansion.

The CC1354P10 wireless MCU has a 48 MHz Arm® Cortex®-M33 CPU and supports 2.4 GHz
as well as sub 1 GHz bands. It has 256KB of SRAM and 1024 KB of FLASH program
memory.

See the `TI CC1354P10 Product Page`_ for additional details.

Supported Features
==================

.. zephyr:board-supported-hw::

Boosterpack Connections
=======================

The most of the I/Os of the MCU are accessible through the 40-pin BoosterPack
connector.

=================+===========+===========================+
| Boosterpack Pin |    Label    |     Function              |
+=================+===========+============================+
|         1       |    3V3      |   3.3V power supply       |
=================+===========+============================+
|         2       |    DIO23    |         A0                |
=================+===========+============================+
|         3       |    DIO12    |       UART0_TX            |
=================+===========+============================+
|         4       |    DIO13    |       UART0_RX            |
=================+===========+============================+
|         5       |    DIO22    |       GPIO                |
=================+===========+============================+
|         6       |    DIO24    |         A1                |
=================+===========+============================+
|         7       |    DIO10    |       SPI_SCLK            |
=================+===========+============================+
|         8       |    DIO21    |       GPIO                |
=================+===========+============================+
|         9       |    DIO4     |       I2C_SCL             |
=================+===========+============================+
|         10      |    DIO5     |       I2C_SDA             |
=================+===========+============================+
|         11      |    DIO15    |       Button 1            |
=================+===========+============================+
|         12      |    DIO41    |       GPIO                |
=================+===========+============================+
|         13      |    DIO42    |       GPIO                |
=================+===========+============================+
|         14      |    DIO8     |       DIO8_MISO           |
=================+===========+============================+
|         15      |    DIO9     |       DIO9_MOSI           |
=================+===========+============================+
|         16      |    LPRST    |       Launchpad reset     |
=================+===========+============================+
|         17      |     DIO14   |       Button 2            |
=================+===========+============================+
|         18      |     DIO11   |       GPIO                |
=================+===========+============================+
|         19      |     DIO19   |       GPIO                |
=================+===========+============================+
|         20      |     GND     |       Ground              |
=================+===========+============================+
|         21      |     5V      |       5V Power            |
=================+===========+============================+
|         22      |     GND     |       Ground              |
=================+===========+============================+
|         23      |     DIO25   |         A2                |
=================+===========+============================+
|         24      |     DIO26   |         A3                |
=================+===========+============================+
|         25      |     DIO27   |         A4                |
=================+===========+============================+
|         26      |     DIO28   |         A5                |
=================+===========+============================+
|         27      |     DIO29   |         A6                |
=================+===========+============================+
|         28      |     DIO30   |         A7                |
=================+===========+============================+
|         29      |     DIO43   |       GPIO                |
=================+===========+============================+
|         30      |     DIO44   |       GPIO                |
=================+===========+============================+
|         31      |     DIO17   |       GPIO/TDI            |
=================+===========+============================+
|         32      |     DIO16   |       GPIO/TDO            |
=================+===========+============================+
|         33      |     TCK     |       TCK                 |
=================+===========+============================+
|         34      |     TMS     |       TMS                 |
=================+===========+============================+
|         35      |     BPRST   |   Boosterpack  reset      |
=================+===========+============================+
|         36      |     DIO18   |       GPIO/RTS/SWO        |
=================+===========+============================+
|         37      |     DIO45   |       GPIO                |
=================+===========+============================+
|         38      |     DIO20   |       GPIO                |
=================+===========+============================+
|         39      |     DIO46   |       GPIO                |
=================+===========+============================+
|         40      |     DIO47   |       GPIO                |
+=================+===========+============================+

Programming and Debugging
*************************

The LP_EM_CC1354P10_1 requires an external debug probe such as the LP-XDS110 or
LP-XDS110ET to be connected for debugging.

Alternatively a J-Link debugger can be connected to the J4 header, along with a
3.3V power supply connected to the pin header.

If not using an XDS110/XDS110-ET debugger, connect an external USB to UART converter
to the following pins to access the serial console.

+----+---------+
| TX |  DIO13  |
+----+---------+
| RX |  DIO12  |
+----+---------+

The serial console can be accessed using a terminal application such as minicom

.. code-block:: console

    $ minicom -D <tty_device> -b <baud_rate>

Replace :code:`<tty_device>` with port of your XDS110/XDS110-ET/debugger.
For example, :code:`/dev/ttyUB0`.

Flashing and debugging
======================

Currently, there is no support for ``CC1354P10 LaunchPad`` in the latest
version of openOCD.

Use CCS (Code Composer Studio) projectless debugging feature available with Code
Composer Studio for flashing and debugging the firmware. It is suggested to use CCSv12
for flashing and debugging programs. Follow these steps to flash and debug firmware
using CCS projectless debugging in CCSv12:

1. Create a new target configuration file by clicking on 'File->New->
   Target Configuration File' and save the target configuration file.

2. Select Connection as 'Texas Instruments XDS110 USB Debug Probe' and
   'Board or Device' as CC1354P10 and save the target configuration file.

3. Click on 'Test Connection' to check if the debugger is able to connect
   with the device.

4. Click on the 'Debug' symbol to start debugging. In the 'Debug' window, right
   click on 'Texas Instruments XDS110 Debug Probe_0/Cortex_M33_0' and click on
   'Connect Target'. Now, the debugger is connected to the target.

5. To load the firmware, click on 'Run->Load->Load Program' and select the
   required firmware file to be loaded.

References
**********

CC1354P10 LaunchPad Quick Start Guide:
 https://www.ti.com/lit/ml/spruiz8/spruiz8.pdf

.. _TI CC1354P10 LaunchPad Product Page:
   https://www.ti.com/tool/LP-EM-CC1354P10

.. _TI CC1354P10 Product Page:
   https://www.ti.com/product/CC1354P10
