.. zephyr:board:: versal_apu

Overview
********
This configuration provides support for the APU (Application Processing Unit) on AMD
Versal devices. The APU can operate as follows:

* Two independent Cortex-A72 cores

This processing unit is based on an ARM Cortex-A72 CPU, and it enables the following devices:

* ARM GIC v3 Interrupt Controller
* ARMv8 Generic Timer
* SBSA UART

Hardware
********
Supported Features
==================

.. zephyr:board-supported-hw::

Devices
=======
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
which points to the DDR memory. The OCM memory area is currently available
for usage, although nothing is placed there by default.

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Only the first CPU in the APU subsystem is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

This board supports two deployment targets:

QEMU Emulation
==============

For QEMU target, XSDB (Xilinx System Debugger) is not used and therefore PDI (Programmable Device Image) is not required. QEMU provides direct emulation without needing hardware initialization files.


.. note::

   **Known QEMU Limitation with TF-A:**
   When running with ARM Trusted Firmware (TF-A) on QEMU, there is a known limitation where
   only simple applications like hello_world work by default. For other applications to work
   properly with QEMU, you need to disable the secure/non-secure transition by adding
   ``CONFIG_ARMV8_A_NS=n`` to your configuration.

   This limitation does not affect real hardware deployment.

Build and run with QEMU:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versal_apu
   :goals: build run

Alternatively, you can build and run separately:

.. code-block:: console

   west build -b versal_apu samples/hello_world
   west build -t run

Real Hardware
=============

For deployment on real Versal hardware, XSDB and a PDI file are required. The PDI file contains the hardware initialization and boot configuration needed for the physical device.

Build the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versal_apu
   :goals: build

Flash to real hardware with PDI file:

.. code-block:: console

   west flash --runner xsdb --pdi /path/to/your.pdi --bl31 /path/to/your_bl31.elf

You should see the following message on the console:

.. code-block:: console

   Hello World! versal_apu/versal_apu


References
**********

1. ARMv8-A Architecture Reference Manual (ARM DDI 0487)
2. Arm Cortex-A72 Core Technical Reference Manual (Doc ID 100095)
3. AMD Versal Technical Reference Manual
