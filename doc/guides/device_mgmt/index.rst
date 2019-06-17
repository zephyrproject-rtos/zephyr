.. _device_mgmt:

Device Management
#################

Overview
********

The management subsystem allows remote management of Zephyr-enabled devices.
The following management operations are available:

* Image management
* File System management
* Log management (currently disabled)
* OS management

over the following transports:

* BLE (Bluetooth Low Energy)
* Serial (UART)

The management subsystem is based on the Simple Management Protocol (SMP)
provided by `MCUmgr`_, an open source project that provides a
management subsystem that is portable across multiple real-time operating
systems.

The management subsystem is split in two different locations in the Zephyr tree:

* `zephyrproject-rtos/mcumgr repo <https://github.com/zephyrproject-rtos/mcumgr>`_
  contains a clean import of the MCUmgr project
* :zephyr_file:`subsys/mgmt/` contains the Zephyr-specific bindings to MCUmgr

Additionally there is a :ref:`sample <smp_svr_sample>` that provides management
functionality over BLE and serial.

.. _mcumgr_cli:

Command-line Tool
*****************

MCUmgr provides a command-line tool, :file:`mcumgr`,for managing remote devices.
The tool is written in the Go programming language.

Installation information can be found in the `MCUmgr command-line tool`_
section of the `MCUmgr documentation`_. A full rundown of the necessary commands
to perform Device Firmware Upgrade is located in
`MCUmgr command-line tool examples`_, and a practical step-by-step guide to DFU
using MCUmgr can be found in :ref:`smp_svr_sample`.

Bootloader integration
**********************

The :ref:`dfu` subsystem integrates the management subsystem with the
bootloader, providing the ability to send and upgrade a Zephyr image to a
device.

Currently only the MCUboot bootloader is supported. See :ref:`mcuboot` for more
information.

.. _MCUmgr: https://github.com/apache/mynewt-mcumgr
.. _MCUmgr documentation: https://github.com/apache/mynewt-mcumgr#mcumgr
.. _MCUmgr command-line tool: https://github.com/apache/mynewt-mcumgr#command-line-tool
.. _MCUmgr command-line tool examples: https://github.com/apache/mynewt-mcumgr-cli#examples
