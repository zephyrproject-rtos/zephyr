.. zephyr:board:: versalnet_rpu

Overview
********
This configuration provides support for the RPU(R52), real-time processing unit on Xilinx
Versal Net SOC, it can operate as following:

* Two independent R52 cores with their own TCMs (tightly coupled memories)
* Or as a single dual lock step unit with the TCM.

This processing unit is based on an ARM Cortex-R52 CPU, it also enables the following devices:

* ARM GIC v3 Interrupt Controller
* Global Timer Counter
* SBSA UART

Hardware
********
Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Timer
------------

This board configuration uses a system timer tick frequency of 100 MHz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
on-chip UART0.

Memories
--------

Although Flash, DDR and OCM memory regions are defined in the DTS file,
all the code plus data of the application will be loaded in the sram0 region,
which points to the DDR memory. The ocm0 memory area is currently available
for usage, although nothing is placed there by default.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Only the first core of the R52 subsystem is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

This board supports two deployment targets:

QEMU Emulation
==============

For QEMU target, XSDB (Xilinx System Debugger) is not used and therefore PDI
(Programmable Device Image) is not required. QEMU provides direct emulation
without needing hardware initialization files.

Build and run with QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versalnet_rpu
   :goals: build run

Alternatively, you can build and run separately:

.. code-block:: console

   west build -b versalnet_rpu samples/hello_world
   west build -t run

Real Hardware
=============

For deployment on real Versal Net hardware, XSDB and a PDI file are required.
The PDI file contains the hardware initialization and boot configuration needed
for the physical device.

Build the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versalnet_rpu
   :goals: build

Flash to real hardware with PDI file:

.. code-block:: console

   west flash --runner xsdb --pdi /path/to/your.pdi

You should see the following message on the console:

.. code-block:: console

   Hello World!


References
**********

1. ARMv8-R Architecture Reference Manual (ARM DDI 0568A.c ID110520)
2. Cortex-R52 and Cortex-R52F Technical Reference Manual (ARM DDI r1p4 100026_0104_01_en)
