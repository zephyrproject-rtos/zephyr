.. _cc26x2r1_launchxl:

CC26x2R1 LaunchXL
#################

Overview
********

The Texas Instruments CC26x2R LaunchPad |trade| (LAUNCHXL-CC26X2R1) is a
development kit for the SimpleLink |trade| multi-Standard CC2652R wireless MCU.

See the `TI CC26x2R LaunchPad Product Page`_ for details.

.. figure:: img/cc26x2r1_launchxl.jpg
   :align: center
   :alt: TI CC26x2R LaunchPad

   Texas Instruments CC26x2R LaunchPad |trade|

Hardware
********

The CC26x2R LaunchPad |trade| development kit features the CC2652R wireless MCU.
The board is equipped with two LEDs, two push buttons and BoosterPack connectors
for expansion. It also includes an integrated (XDS110) debugger.

The CC2652 wireless MCU has a 48 MHz Arm |reg| Cortex |reg|-M4F SoC and an
integrated 2.4 GHz transceiver supporting multiple protocols including Bluetooth
|reg| Low Energy and IEEE |reg| 802.15.4.

See the `TI CC2652R Product Page`_ for additional details.

Supported Features
==================

The CC26x2R LaunchPad board configuration supports the following hardware
features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PINMUX    | on-chip    | pinmux               |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| I2C       | on-chip    | i2c                  |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+
| AUX_ADC   | on-chip    | adc                  |
+-----------+------------+----------------------+
| HWINFO    | on-chip    | hwinfo               |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

All I/O signals are accessible from the BoosterPack connectors. Pin function
aligns with the LaunchPad standard.

+-------+-----------+---------------------+
| Pin   | Function  | Usage               |
+=======+===========+=====================+
| DIO0  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO1  | GPIO      |                     |
+-------+-----------+---------------------+
| DIO2  | UART0_RX  | UART RXD            |
+-------+-----------+---------------------+
| DIO3  | UART0_TX  | UART TXD            |
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
| DIO12 | GPIO      |                     |
+-------+-----------+---------------------+
| DIO13 | GPIO      | Button 1            |
+-------+-----------+---------------------+
| DIO14 | GPIO      | Button 2            |
+-------+-----------+---------------------+
| DIO15 | GPIO      |                     |
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

Before flashing or debugging ensure the RESET, TMS, TCK, TDO, and TDI jumpers
are in place. Also place jumpers on the the TXD and RXD signals for a serial
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

   You can obtain OpenOCD by following these
   :ref:`installing the latest Zephyr SDK instructions <toolchain_zephyr_sdk>`.

   After the installation, add the directory containing the OpenOCD executable
   to your environment's PATH variable. For example, use this command in Linux:

   .. code-block:: console

      export PATH=$ZEPHYR_SDK_INSTALL_DIR/sysroots/x86_64-pokysdk-linux/usr/bin/openocd:$PATH

Flashing
========

Applications for the ``CC26x2R LaunchPad`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ screen <tty_device> 115200

Replace :code:`<tty_device>` with the port where the XDS110 application
serial device can be found. For example, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cc26x2r1_launchxl
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: cc26x2r1_launchxl
   :maybe-skip-config:
   :goals: debug

Bootloader
==========

The ROM bootloader on CC13x2 and CC26x2 devices is enabled by default. The
bootloader will start if there is no valid application image in flash or the
so-called backdoor is enabled (via option
:kconfig:option:`CONFIG_CC13X2_CC26X2_BOOTLOADER_BACKDOOR_ENABLE`) and BTN-1 is held
down during reset. See the bootloader documentation in chapter 10 of the `TI
CC13x2 / CC26x2 Technical Reference Manual`_ for additional information.

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

CC26X2R1 LaunchPad Quick Start Guide:
  http://www.ti.com/lit/pdf/swru528

.. _TI CC26x2R LaunchPad Product Page:
   http://www.ti.com/tool/launchxl-cc26x2r1

.. _TI CC2652R Product Page:
   http://www.ti.com/product/cc2652r

.. _TI CC26x2R LaunchPad Quick Start Guide:
   http://www.ti.com/lit/pdf/swru528

.. _TI CC2652R Datasheet:
   http://www.ti.com/lit/pdf/swrs207

.. _TI CC13x2 / CC26x2 Technical Reference Manual:
   http://www.ti.com/lit/pdf/swcu185

..  _XDS-110 emulation package:
   http://processors.wiki.ti.com/index.php/XDS_Emulation_Software_Package#XDS_Emulation_Software_.28emupack.29_Download
