.. zephyr:board:: cc1352p7_lp

Overview
********

The Texas Instruments CC1352P7 LaunchPad |trade| (LP-CC1352P7) is a
development kit for the SimpleLink |trade| multi-Standard CC1352P7 wireless MCU.

See the `TI CC1352P7 LaunchPad Product Page`_ for details.


Hardware
********

The CC1352P7 LaunchPad |trade| development kit features the CC1352P7 wireless MCU.
The board is equipped with two LEDs, two push buttons, antenna switch and
BoosterPack connectors for expansion. It also includes an integrated (XDS110)
debugger.

The CC1352P7 wireless MCU has a 48 MHz Arm |reg| Cortex |reg|-M4F SoC and an
integrated sub-1GHz and 2.4 GHz transceiver with integrated 20dBm power amplifier
(PA) supporting multiple protocols including Bluetooth |reg| Low Energy and IEEE
|reg| 802.15.4.

See the `TI CC1352P7 Product Page`_ for additional details.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

All I/O signals are accessible from the BoosterPack connectors. Pin function
aligns with the LaunchPad standard.

+-------+-----------+---------------------+
| Pin   | Function  | Usage               |
+=======+===========+=====================+
| DIO3  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO4  | I2C_MSSCL | I2C SCL             |
+-------+-----------+---------------------+
| DIO5  | I2C_MSSDA | I2C SDA             |
+-------+-----------+---------------------+
| DIO6  | GPIO      | Red LED             |
+-------+-----------+---------------------+
| DIO7  | GPIO      | Green LED           |
+-------+-----------+---------------------+
| DIO8  | SSI0_RX   | SPI MISO            |
+-------+-----------+---------------------+
| DIO9  | SSI0_TX   | SPI MOSI            |
+-------+-----------+---------------------+
| DIO10 | SSI0_CLK  | SPI CLK             |
+-------+-----------+---------------------+
| DIO11 | SSIO_CS   | SPI CS              |
+-------+-----------+---------------------+
| DIO12 | UART0_RX  | UART RXD            |
+-------+-----------+---------------------+
| DIO13 | UART0_TX  | UART TXD            |
+-------+-----------+---------------------+
| DIO14 | GPIO      | Button 2            |
+-------+-----------+---------------------+
| DIO15 | GPIO      | Button 1            |
+-------+-----------+---------------------+
| DIO16 |           | JTAG TDO            |
+-------+-----------+---------------------+
| DIO17 |           | JTAG TDI            |
+-------+-----------+---------------------+
| DIO18 | UART0_RTS | UART RTS / JTAG SWO |
+-------+-----------+---------------------+
| DIO19 | UART0_CTS | UART CTS            |
+-------+-----------+---------------------+
| DIO20 | GPIO      | Flash CS            |
+-------+-----------+---------------------+
| DIO21 | GPIO      |                     |
+-------+-----------+---------------------+
| DIO22 | GPIO      |                     |
+-------+-----------+---------------------+
| DIO23 | AUX_IO    | A0                  |
+-------+-----------+---------------------+
| DIO24 | AUX_IO    | A1                  |
+-------+-----------+---------------------+
| DIO25 | AUX_IO    | A2                  |
+-------+-----------+---------------------+
| DIO26 | AUX_IO    | A3                  |
+-------+-----------+---------------------+
| DIO27 | AUX_IO    | A4                  |
+-------+-----------+---------------------+
| DIO28 | AUX_IO    | A5                  |
+-------+-----------+---------------------+
| DIO29 | AUX_IO    | A6                  |
+-------+-----------+---------------------+
| DIO30 | AUX_IO    | A7                  |
+-------+-----------+---------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Before flashing or debugging ensure the RESET, TMS, TCK, TDO, and TDI jumpers
are in place. Also place jumpers on the TXD and RXD signals for a serial
console using the XDS110 application serial port.

Prerequisites:
==============

#. Ensure the XDS-110 emulation firmware on the board is updated.

   Download and install the latest `XDS-110 emulation package`_.

   Follow these `xds110 firmware update directions
   <http://software-dl.ti.com/ccs/esd/documents/xdsdebugprobes/emu_xds110.html#updating-the-xds110-firmware>`_

   Note that the emulation package install may place the xdsdfu utility
   in ``<install_dir>/ccs_base/common/uscif/xds110/``.

#. Install OpenOCD

   Currently, OpenOCD doesn't support the CC1352P7.
   Until its support get merged, we have to builld a downstream version that could found `here <https://github.com/anobli/openocd>`_.
   Please refer to OpenOCD documentation to build and install OpenOCD.

   For your convenience, we provide a `prebuilt binary <https://github.com/anobli/openocd/actions/runs/10566225265>`_.

.. code-block:: console

   $ unzip openocd-810cb5b21-x86_64-linux-gnu.zip
   $ chmod +x openocd-x86_64-linux-gnu/bin/openocd
   $ export OPENOCD_DIST=$PWD/openocd-x86_64-linux-gnu

By default, zephyr will try to use the OpenOCD binary from SDK.
You will have to define :code:`OPENOCD` and :code:`OPENOCD_DEFAULT_PATH` to use the custom OpenOCD binary.

Flashing
========

Applications for the ``CC1352P7 LaunchPad`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ screen <tty_device> 115200

Replace :code:`<tty_device>` with the port where the XDS110 application
serial device can be found. For example, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cc1352p7_lp
   :goals: build flash
   :gen-args: -DOPENOCD=$OPENOCD_DIST/bin/openocd -DOPENOCD_DEFAULT_PATH=$OPENOCD_DIST/share/openocd

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cc1352p7_lp
   :maybe-skip-config:
   :goals: debug
   :gen-args: -DOPENOCD=$OPENOCD_DIST/bin/openocd -DOPENOCD_DEFAULT_PATH=$OPENOCD_DIST/share/openocd

Bootloader
==========

The ROM bootloader on CC13x2x7 and CC26x2x7 devices is enabled by default. The
bootloader will start if there is no valid application image in flash or the
so-called backdoor is enabled (via option
:kconfig:option:`CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_ENABLE`) and BTN-1 is held
down during reset. See the bootloader documentation in chapter 10 of the `TI
CC13x2x7 / CC26x2x7 Technical Reference Manual`_ for additional information.

Power Management and UART
=========================

System and device power management are supported on this platform, and
can be enabled via the standard Kconfig options in Zephyr, such as
:kconfig:option:`CONFIG_PM`, :kconfig:option:`CONFIG_PM_DEVICE`.

When system power management is turned on (CONFIG_PM=y),
sleep state 2 (standby mode) is allowed, and polling is used to retrieve input
by calling uart_poll_in(), it is possible for characters to be missed if the
system enters standby mode between calls to uart_poll_in(). This is because
the UART is inactive while the system is in standby mode. The workaround is to
disable sleep state 2 while polling:

.. code-block:: c

    pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
    <code that calls uart_poll_in() and expects input at any point in time>
    pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);


References
**********

CC1352P7 LaunchPad Quick Start Guide:
  https://www.ti.com/lit/pdf/swru573

.. _TI CC1352P7 LaunchPad Product Page:
   https://www.ti.com/tool/LP-CC1352P7

.. _TI CC1352P7 Product Page:
   https://www.ti.com/product/CC1352P7

.. _TI CC13x2x7 / CC26x2x7 Technical Reference Manual:
   https://www.ti.com/lit/ug/swcu192/swcu192.pdf

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS_Emulation_Software_.28emupack.29_Download
