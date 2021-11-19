.. _cc3235s_launchxl:

CC3235S LaunchXL
################

Overview
********

The SimpleLink Wi-Fi CC3235S LaunchPad development kit (CC3235S-LAUNCHXL)
highlights CC3235S, a single-chip wireless microcontroller (MCU) with 4MB
external serial flash, 256KB of RAM, and enhanced security features. It
supports 802.11 a/b/g/n, both 2.4 GHz and 5 GHz.

See the `TI CC3235 Product Page`_ for details.

Most aspects of the board are described in its sister board :ref:`cc3235sf_launchxl`.

.. figure:: cc3235s_launchxl.png
   :width: 800px
   :align: center
   :alt: CC3235S LaunchXL

   CC3235S LaunchXL

For a board overview refer to `LaunchPad Overview`_ and you can find the pinout
on `LaunchPad Pinout`_.

Hardware
********

The CC3235S SoC has two MCUs:

#. Applications MCU - an ARM |reg| Cortex |reg|-M4 Core at 80 MHz, with 256Kb RAM,
   and access to external serial 4MB flash with bootloader and peripheral
   drivers in ROM.

#. Network Coprocessor (NWP) - a dedicated ARM MCU, which completely
   offloads Wi-Fi and internet protocols from the application MCU.

Complete details of the CC3235S SoC can be found in the `CC3235 TRM`_.

Supported Features
==================

Zephyr has been ported to the Applications MCU, with basic peripheral
driver support.

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| UART      | on-chip    | serial port-interrupt |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| I2C       | on-chip    | i2c                   |
+-----------+------------+-----------------------+
| SPI_0     | on-chip    | Wi-Fi host driver     |
+-----------+------------+-----------------------+

.. note::

   For consistency with TI SimpleLink SDK and BoosterPack examples,
   the I2C driver defaults to I2C_BITRATE_FAST mode (400 kHz) bus speed
   on bootup.

The accelerometer, temperature sensors, or other peripherals
accessible through the BoosterPack, are not currently supported.

Connections and IOs
===================

Peripherals on the CC3235S LaunchXL are mapped to the following pins in
the file :zephyr_file:`boards/arm/cc3235s_launchxl/pinmux.c`.

+------------+-------+-------+
| Function   | PIN   | GPIO  |
+============+=======+=======+
| UART0_TX   | 55    | N/A   |
+------------+-------+-------+
| UART0_RX   | 57    | N/A   |
+------------+-------+-------+
| LED D7 (R) | 64    | 9     |
+------------+-------+-------+
| LED D6 (O) | 01    | 10    |
+------------+-------+-------+
| LED D5 (G) | 02    | 11    |
+------------+-------+-------+
| Switch SW2 | 15    | 22    |
+------------+-------+-------+
| Switch SW3 | 04    | 13    |
+------------+-------+-------+

The default configuration can be found in the Kconfig file at
:zephyr_file:`boards/arm/cc3235s_launchxl/cc3235s_launchxl_defconfig`.


Programming and Debugging
*************************

Flashing
========

TI officially supports development on the CC3235S using the TI
`CC32xx SDK`_ on Windows and Linux using TI tools: Code Composer
Studio for debugging and `UniFlash`_ for flashing, described here: `UniFlash
Basics`_. A new way of flashing is to use SysConfig, please see `SysConfig
ImageCreator`_ for details. SysConfig is also TI's supported tool for hardware
configuration, refer the `SysConfig Basics`_ to learn how to use it.

There are two different variants of images: ``Production`` and ``Development``,
and only when development images accept JTAG connections. The downside with the
development mode, is that then the image must be locked to a specific MAC
address.

No matter what variant is flashed, the CC3235S on-chip bootloader only accepts
signed images, i.e. the image must be signed with a private key and can be
verified with the public key installed in the flash file system (User Files).

SysConfig can be used as standalone tool or embedded in Code Composer Studio.
It can be started in two different Modes: "Default Mode" and "Image Creator Mode".
The default mode is for configuring the hardware. The default mode is for
setting up the image creation, usually in the file ``image.syscfg``. Internally
it will use the tool ``ImageCreator`` for creating the SLI files in the last
step `SysConfig ImageCreator`_. To start a new development image, follow these
steps:

#. Start the SysConfig standalone Tool
#. Configure the Image Creator Mode and the Device as follows

   .. figure:: sysconfig-start.png

#. After starting, go to ``NETWORK PROCESSOR`` > ``General Settings``,
   press ``+ ADD``, and choose the desired Image Mode.
#. Untick Auto-Detect Target, this does only work with the Launchpads at the time writing.
#. Enter the device MAC address.
#. Optional: Adjust the flash capacity

   .. figure:: sysconfig-general.png

#. Copy the key chain from ``<sdk>/tools/cc32xx_tools/certificate-playground``
   to the directory ``user-files`` in the repository:
   * ``dummy-root-ca-cert``      -> ``user-files/``
   * ``dummy-trusted-ca-cert``   -> ``user-files/``
   * ``dummy-trusted-cert``      -> ``user-files/``
#. Copy the signing key to the directory ``sysconfig`` in the repository:
   * ``dummy-trusted-cert-key``  -> ``sysconfig/``
#. Now we fill the Flash File System. Therefore go to
   ``FILE SYSTEM`` > ``Certificate Store`` press ``+ ADD`` and choose as
   Certificate Catalog ``Use Dummy Root Certificate Playground from SDK``.
#. Go to ``MCU Image``, press ``+ ADD``, and add the ``*.bin`` file of the firmware image, the
   ``Private Key`` ``sysconfig/dummy-trusted-cert-key`` and the ``Certificate``
   ``user-files/dummy-trusted-cert``.

   .. figure:: sysconfig-fs.png

#. Go to ``User Files``, press ``+ ADD``, and select ``user-files`` as
   ``Root Directory``.
#. Now, go to ``IMAGE CREATION`` > ``Image Commands``, press ``+ ADD`` and select
   ``Add manually`` in ``Choose SLI File``.
#. Enter manually the full path of output file in ``Image File (SLI)``. The
   generated SLI file can be flashed to only this device with the specified
   MAC address.

Afterwards flash the ``SLI`` file with SysConfig or with UniFlash, but UniFlash seemed
to work more reliably.

Debugging
=========

To debug a previously flashed image, after resetting the board, use the 'debug'
build target:

.. note:: JTAG is only unlocked when a development image is flashed.

.. zephyr-app-commands::
   :zephyr-app: <my_app>
   :board: cc3235s_launchxl
   :maybe-skip-config:
   :goals: debug

References
**********

.. SOC

.. _TI SimpleLink MCUs:
    http://www.ti.com/microcontrollers/simplelink-mcus/overview.html

.. _TI CC3235 Product Page:
    http://www.ti.com/product/cc3235S

.. _CC3235 TRM:
   http://www.ti.com/lit/pdf/swru543

.. _CC3x20/CC3x35 SimpleLink Wi-Fi and IoT Network Processor Programmer's Guide:
   http://www.ti.com/lit/pdf/swru455

.. _CC32xx Quick Start Guide:
   http://software-dl.ti.com/ecs/SIMPLELINK_CC32XX_SDK/5_20_00_06/exports/docs/simplelink_mcu_sdk/Quick_Start_Guide.html

.. Board

.. _CC3235 LaunchPad Out of Box Experience:
   https://dev.ti.com/tirex/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/wifi/cc3235_launchpad_oobe/cc3235_launchpad_oobe.html

.. _CC3235S LaunchPad Dev Kit Hardware User's Guide:
   http://www.ti.com/lit/pdf/swru539

.. _LaunchPad Overview:
   https://dev.ti.com/tirex/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/wifi/cc3235_launchpad_oobe/cc3235_launchpad_oobe.html

.. _LaunchPad Pinout:
   https://dev.ti.com/tirex/explore/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/wifi/cc3235_launchpad_oobe/cc3235_launchpad_oobe.html#-your-launchpad-pinout

.. SDK

.. _CC32xx SDK:
   https://www.ti.com/tool/download/SIMPLELINK-CC32XX-SDK/5.20.00.06

.. _Simplelink Academy:
   https://dev.ti.com/tirex/explore/node?node=ACXX7T0yv567-BCqBKH.uw__fc2e6sr__LATEST

.. _CC32xx SimpleLink Host Driver API Reference:
   http://software-dl.ti.com/ecs/SIMPLELINK_CC32XX_SDK/5_20_00_06/exports/docs/wifi_host_driver_api/html/index.html

.. _CC32xx MCU Driver API Reference:
   http://software-dl.ti.com/ecs/SIMPLELINK_CC32XX_SDK/5_20_00_06/exports/docs/drivers/doxygen/html/index.html

.. Flashing

.. _UniFlash:
   http://www.ti.com/tool/UNIFLASH

.. _CC3x3x UniFlash:
   http://www.ti.com/lit/pdf/swru469h

.. _SysConfig ImageCreator:
   http://dev.ti.com/tirex/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/wifi/wifi_sysconfig_imagecreator/wifi_sysconfig_imagecreator.html

.. _UniFlash Basics:
   https://dev.ti.com/tirex/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/wifi/wifi_imagecreator/wifi_imagecreator.html

.. _SysConfig ImageCreator User Guide:
   http://dev.ti.com/tirex/explore/content/simplelink_cc32xx_sdk_5_20_00_06/docs/simplelink_mcu_sdk/sysconfig_imagecreator.html

.. _SysConfig Basics:
   https://dev.ti.com/tirex/content/simplelink_academy_cc32xxsdk_5_20_00_00/modules/tools/sysconfig_basics/sysconfig_basics.html

License
*******

Copyright (c) 2021 ithinx GmbH

SPDX-License-Identifier: Apache-2.0
