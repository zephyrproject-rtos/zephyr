.. zephyr:board:: versalnet_apu

Overview
********
This configuration provides support for the APU(A78), ARM processing unit on AMD
Versal Net SOC, it can operate as following:

* Four independent A78 clusters each having 4 A78 cores

This processing unit is based on an ARM Cortex-A78 CPU, it also enables the following devices:

* ARM GIC v3 Interrupt Controller
* Global Timer Counter
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
which points to the DDR memory. The ocm0 memory area is currently available
for usage, although nothing is placed there by default.

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Only the first cpu in the first cluster of the A78 subsystem is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash in the usual way. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versalnet_apu
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! versalnet_apu/amd_versalnet_apu


References
**********

1. ARMv8‑A Architecture Reference Manual (ARM DDI 0487)
2. Arm Cortex‑A78 Core Technical Reference Manual (Doc ID 101430)
