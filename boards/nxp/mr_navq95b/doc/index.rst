.. zephyr:board:: mr_navq95b

Overview
********

The `NXP MR-NAVQ95`_ is an open-source development board designed for mobile robotics
applications. It is based on the NXP i.MX95 applications processor and provides
heterogeneous multicore processing capabilities suitable for combining
real-time workloads with high-performance application processing.

The platform integrates Cortex-A55 application cores alongside Cortex-M7 and
Cortex-M33 real-time cores. In Zephyr, support is currently focused on the
Cortex-M7 core, which is typically the target for real-time firmware.


Hardware
********

Processor
=========

- NXP i.MX95 SoC

  - 6x Arm Cortex-A55 cores
  - 1x Arm Cortex-M7 core
  - 1x Arm Cortex-M33 core


Memory
======

- On-chip SRAM: 1376 KiB (ECC)
- External LPDDR5: 16 GiB (with inline ECC and encryption)
- External Flash: 64 MiB Octal SPI NOR


Supported Features
==================

.. zephyr:board-supported-hw::


Connectivity
============

The `NXP MR-NAVQ95`_ consists of a base board which can be extended with optional
add-on modules for additional interfaces and functionality.
This `block diagram`_ provides an overview of the extension boards and the peripherals that
are available for use.

These interfaces are multiplexed and depend on board configuration and pinmux
settings of the i.MX95 platform.


Serial Console
**************

The default serial console is routed to UART2 interface.
It can be accessed through a USB-to_UART bridge on the USB-C connector (J10) on the I/O expansion board.
Alternatively, UART2 is also available on a dedicated JST-GH board connector (J2) on the main board.


Build and run Hello World
*************************

It is recommended to have the NavQ95 image installed on the SD-card. This will initialise the M33 and the A55 cores
and allows to run the zephyr firmware from external flash by running MCUBoot on the M7 at startup.
Build and download instructions for this SD-card image can be found on `NXP IMX-MANIFEST-NAVQ95`_

To build a Zephyr application for the MR-NAVQ95B on ITCM:

.. code-block:: bash

   west build -b mr_navq95b/mimx9596/m7 samples/hello_world/

To build the application to run from the MX25UM51345G external flash:

.. code-block:: bash

   west build -b mr_navq95b/mimx9596/m7/flash samples/hello_world/


Uploading the binary can be performed using a Segger/J-Link probe connected to port J7:

.. code-block:: bash

   west flash


.. note::

   You need to have support for the mimx95_cm7_mx25um target in PyOCD to flash the binary onto the external flash.
   At the time of writing this support is not yet pulled in.

   Follow this procedure to get PyOCD with support for mimx95_cm7_mx25um:

   - Install pyocd to flash the image through the on-board JTAG device.
   - Clone pyocd into a directory of your preference and checkout the pr-imx95 branch:

     .. code-block:: bash

       git clone https://github.com/NXP-Robotics/pyOCD -b pr-imx95

   - Build pyocd:

     .. code-block:: bash

       cd pyocd-private
       python3 -m pip install .


When running the firmware from external flash, the standard boot flow involves:

1. System Manager (Cortex-M33) initializes the system
2. MCUBoot is loaded into TCM
3. MCUBoot validates firmware in external flash
4. Firmware executes from flash (XIP)


.. include:: ../../common/board-footer.rst.inc

References
**********

.. target-notes::

.. _NXP MR-NAVQ95:
   https://github.com/NXP-Robotics/MR-NAVQ95

.. _NXP IMX-MANIFEST-NAVQ95:
   https://github.com/NXP-Robotics/imx-manifest-navq95

.. _block diagram:
   https://github.com/NXP-Robotics/MR-NAVQ95/blob/main/images/mr-navq95b-overview.png
