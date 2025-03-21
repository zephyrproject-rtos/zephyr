..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright 2025 - 2026 Siemens Mobility GmbH
   SPDX-FileCopyrightText: Copyright 2025 Texas Instruments
   SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: lp_am243

Overview
********

The AM243x Launchpad is a development board that is based of a AM2434 SoC and
therefor has two dualcore Cortex-R5F clusters running at 800 MHz and a
Cortex-M4F running at 400 MHz. The board also includes a flash chip,
DIP-Switches for the boot mode selection and 2 RJ45 Ethernet ports.

More information can be found at the `AM243x Launchpad product page`_ and the
`AM2434 SoC website`_.


Hardware
********

.. include:: ../../common/doc/k3/am64x-am243x-features.rst
   :start-after: ti-k3-am64x-am243x-features


The AM243x Launchpad
====================

The AM243x Launchpad uses an AM2434 SoC resulting in 2 dualcore Cortex-R4F
clusters running at 800 MHz in the MAIN domain and a Cortex-M4F running at 400
MHz in the MCU domain.

The board physically contains:

* 512 Mb (equal to 64 MB) of Infineon NOR Flash
* DIP switches to change the boot mode
* Buttons for different reset methods
* Multiple LEDs (some for power indication and some connected to GPIO pins)
* Two physical Ethernet ports including PHYs
* A XDS110 debug probe (JTAG emulation)


The Cortex-R5F0_0 core uses the UART peripheral connected to the XDS110 debug
probe by default and is therefor usable without additional UART to USB bridge.
The Cortex-M4F core uses the MCU_UART0 peripheral by default to avoid conflicts
which is accessible via the J17 header on the board.


Supported Features
==================

.. zephyr:board-supported-hw::


Booting a firmware
==================

.. include:: ../../common/doc/k3/am64x-am243x-bootmodes.rst
   :start-after: ti-k3-am64x-am243x-bootmodes


On this board the location from which the SBL image is fetched can be selected
with the DIP switches.

.. list-table:: UART Boot Mode
   :header-rows: 1

   * - SW4 [1:7]
   * - 11100000

.. list-table:: QSPI Boot Mode
   :header-rows: 1

   * - SW4 [1:7]
   * - 01000100


Flashing
********
The boot process of the AM2434 SoC requires the booting image to be in a
specific format and to wait for the internal DMSC-L of the AM2434 to start up
and configure memory firewalls. Since there exists no Zephyr support it's
required to use one of the SBL bootloader examples from the TI MCU+ SDK.

.. attention::
   For all MCU+ SDK related sections you need to replace ``$Z_MCUPSDK_BOARD``
   with ``am243x-lp`` (or set it as environment variable, if it is not inside a
   file)!

.. note::
   Please be aware that while this board using Quad SPI (QSPI) instead of
   Octal SPI (OSPI) the TI MCU+ SDK still uses QSPI correctly inside the OSPI
   code for this board

.. include:: ../../common/doc/k3/am64x-am243x-mcuplus-sdk.rst
   :start-after: ti-k3-am64x-am243x-mcuplus-sdk


Debugging
*********

OpenOCD
=======

The board is equipped with an XDS110 JTAG debugger. To debug a binary, utilize
the ``debug`` build target:

.. zephyr-app-commands::
   :app: <my_app>
   :board: lp_am243/<soc>/<core>
   :maybe-skip-config:
   :goals: debug

.. hint::
   To utilize this feature, you'll need an OpenOCD version that is new enough
   and therefor might need to compile OpenOCD yourself. You can look at
   `building OpenOCD from source`_ for documentation on how to do that.


References
**********

.. target-notes::

.. _AM243x Launchpad product page:
   https://www.ti.com/tool/LP-AM243

.. _AM2434 SoC website:
   https://www.ti.com/product/en-us/AM2434#tech-docs

.. _building OpenOCD from source:
   https://docs.u-boot.org/en/latest/board/ti/k3.html#building-openocd-from-source
