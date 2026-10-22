.. zephyr:board:: versal_rpu

Overview
********
This configuration provides support for the RPU (Real-time Processing Unit) on AMD
Versal devices, it can operate as following:

* Two independent R5 cores with their own TCMs (tightly coupled memories)
* Or as a single dual lock step unit with the TCM.

This processing unit is based on an ARM Cortex-R5F CPU, it also enables the following devices:

* ARM GIC v2 Interrupt Controller
* Xilinx TTC (Triple Timer Counter)
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

Although Flash, DDR and TCM memory regions are defined in the DTS file,
all the code plus data of the application will be loaded in the sram0 region,
which points to the DDR memory. The TCM memory area is currently available
for usage, although nothing is placed there by default.

Known Problems or Limitations
=============================

The following platform features are unsupported:

* Only the first core of the R5 subsystem is supported.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

For deployment on real Versal hardware, XSDB and a PDI file are required. The PDI file contains the hardware initialization and boot configuration needed for the physical device.

Build the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: versal_rpu
   :goals: build

Flash to real hardware with PDI file:

.. code-block:: console

   west flash --runner xsdb --pdi /path/to/your.pdi

You should see the following message on the console:

.. code-block:: console

   Hello World! versal_rpu/versal_rpu

References
**********

1. ARMv7-A and ARMv7-R Architecture Reference Manual (ARM DDI 0406C ID051414)
2. Cortex-R5 and Cortex-R5F Technical Reference Manual (ARM DDI 0460C ID021511)
3. Versal ACAP Technical Reference Manual (AM011)
