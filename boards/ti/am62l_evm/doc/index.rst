.. zephyr:board:: am62l_evm

Overview
********

The AM62L EVM board configuration is used by Zephyr applications that run on
the TI AM62L platform. The board configuration provides support for:

- ARM Cortex-A53 core and the following features:

   - General Interrupt Controller (GIC)
   - ARM Generic Timer (arch_timer)
   - On-chip SRAM (oc_sram)
   - UART interfaces (uart0 to uart6)

The board configuration also enables support for the semihosting debugging console.

See the `TI AM62L Product Page`_ for details.

Hardware
********
The AM62L EVM features the AM62L SoC, which is composed of a dual Cortex-A53
cluster. The following listed hardware specifications are used:

- High-performance ARM Cortex-A53
- Memory

   - 160KB of SRAM
   - 2GB of DDR4

- Debug

   - XDS110 based JTAG

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Clock
------------

This board configuration uses a system clock frequency of 1250 MHz.

DDR RAM
-------

The board has 2GB of DDR RAM available.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
MAIN domain UART (main_uart0).

SD Card
*******

Download TI's official `WIC`_ and flash the WIC file with an etching software
onto an SD-card.

Copy the compiled ``zephyr.bin`` to the first FAT partition of the SD card and
plug the SD card into the board. Power it up and stop the u-boot execution at
prompt.

Use U-Boot to load and start zephyr.bin:

.. code-block:: console

    fatload mmc 1:1 0x82000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x82000000

The Zephyr application should start running on the A53 core.

Debugging
*********

The board is equipped with an XDS110 JTAG debugger. To debug a binary, utilize the ``debug`` build target:

.. zephyr-app-commands::
   :app: <my_app>
   :board: am62l_evm/am62l3/a53
   :maybe-skip-config:
   :goals: debug

.. hint::
   To utilize this feature, you'll need OpenOCD version 0.12 or higher. Due to the possibility of
   older versions being available in package feeds, it's advisable to `build OpenOCD from source`_.

References
**********

https://www.ti.com/tool/TMDS62LEVM

.. _AM62L EVM TRM:
   https://www.ti.com/lit/pdf/sprujb4

.. _TI AM62L Product Page:
   https://www.ti.com/product/AM62L

.. _WIC:
   https://dr-download.ti.com/software-development/software-development-kit-sdk/MD-YjEeNKJJjt/11.02.08.02/tisdk-default-image-am62lxx-evm-11.02.08.02.rootfs.wic.xz

.. _EVM User's Guide:
   https://www.ti.com/lit/pdf/SPRUJG8

.. _build OpenOCD from source:
   https://docs.u-boot.org/en/latest/board/ti/k3.html#building-openocd-from-source
