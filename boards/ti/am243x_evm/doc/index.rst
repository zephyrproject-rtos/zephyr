..
   SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
   SPDX-FileCopyrightText: Copyright 2025 - 2026 Siemens Mobility GmbH
   SPDX-FileCopyrightText: Copyright 2025 Texas Instruments
   SPDX-License-Identifier: Apache-2.0

.. zephyr:board:: am243x_evm

Overview
********

The AM243x EVM is a development board that is based of a AM2434 SoC. The
Cortex R5F cores in the SoC run at 800 MHz. The board also includes a flash
region, DIP-Switches for the boot mode selection and 2 RJ45 Ethernet ports.

See the `TI TMDS243EVM Product Page`_ or its `technical reference manual <AM243x
EVM TRM_>`_ for details.

Hardware
********

.. include:: ../../common/doc/k3/am64x-am243x-features.rst
   :start-after: ti-k3-am64x-am243x-features


The AM243x-EVM board
====================
This development board specifically uses an AM2434 SoC resulting in 4 Cortex-R5F
cores and a Cortex-M4F core. These Cortex-R5F cores run at 800 MHz and the
Cortex-M4F core runs at 400 MHz.

On the board it additionally features

- 2GB DDR4 RAM
- XDS110 based JTAG

Inside Zephyr it configures

- a 4KB Resource Table at 0xa4100000 for the M4
- a 4KB Resource Table at 0xa0100000 for the R5F0_0
- 8MB of Shared Memory at 0xa5000000 for inter-processor communication
- MAIN domain UART0 for the R5F0_0
- MCU domain UART0 (MCU_UART0) for the M4


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

   * - SW2 [0:7]
     - SW3 [8:15]
   * - 11011100
     - 10110000

.. list-table:: OSPI Boot Mode
   :header-rows: 1

   * - SW2 [0:7]
     - SW3 [8:15]
   * - 11001110
     - 01000000


Flashing
********
The boot process of the AM2434 SoC requires the booting image to be in a
specific format and to wait for the internal DMSC-L of the AM2434 to start up
and configure memory firewalls. Since there exists no Zephyr support it's
required to use one of the SBL bootloader examples from the TI MCU+ SDK.

.. attention::
   For all MCU+ SDK related sections you need to replace ``$Z_MCUPSDK_BOARD``
   with ``am243x-evm`` (or set it as environment variable, if it is inside a
   file)!

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
   :board: am243x_evm/<soc>/<core>
   :maybe-skip-config:
   :goals: debug

.. hint::
   To utilize this feature, you'll need OpenOCD version 0.12 or higher. Due to the possibility of
   older versions being available in package feeds, it's advisable to `build OpenOCD from source`_.


References
**********


.. target-notes::

.. _AM243x EVM TRM:
   https://www.ti.com/lit/ug/spruj63a/spruj63a.pdf

.. _TI TMDS243EVM Product Page:
   https://www.ti.com/tool/TMDS243EVM

.. _build OpenOCD from source:
   https://docs.u-boot.org/en/latest/board/ti/k3.html#building-openocd-from-source
