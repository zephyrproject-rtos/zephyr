.. zephyr:board:: mbv32

Overview
********

This configuration provides support for the AMD MicroBlaze V design preset from
the AMD Vivado™ Design Suite, targeted at the AMD Kintex UltraScale FPGA KCU105
evaluation kit. The preset uses the realtime configuration, with fast interrupts
disabled. It enables the following devices:

* Xilinx AXI Interrupt Controller
* AMD AXI Timer
* Xilinx AXI UARTLite
* Xilinx AXI GPIO

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Devices
=======

System Timer
------------

This board configuration uses the AMD AXI Timer as the system timer.
The timer is driven by a 100 MHz clock.

Serial Port
-----------

This board configuration uses the Xilinx AXI UARTLite for the serial
console at 115200 baud.

Interrupt Controller
--------------------

This board configuration uses the Xilinx AXI Interrupt Controller as
a second-level interrupt controller.

Memories
--------

The board provides 128 KB of local BRAM and 2 GB of DDR memory.
The ``zephyr,sram`` chosen node points to the DDR ``ram0`` region,
so application code and data are placed in DDR by default. The local
BRAM region is available for use but is not used by default.

Known Problems or Limitations
=============================

The following platform features are currently unsupported:

* The Vivado preset contains AXI I2C and AXI QSPI IP, but this board
  configuration does not enable them.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

A prebuilt bitstream is available for this design. ``west flash`` programs
the FPGA with the bitstream and then loads the Zephyr application.

Build the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mbv32
   :goals: build

Flash to real hardware using the bitstream:

.. code-block:: console

   west flash --runner xsdb --bitstream <path>/system.bit

You should see the following message on the console:

.. code-block:: console

   *** Booting Zephyr OS build <version> ***
   Hello World! mbv32/mbv32

References
**********

* `AMD MicroBlaze™ V Processor`_
* `AMD Vivado™ Design Suite`_
* `AMD Kintex UltraScale FPGA KCU105 Evaluation Kit`_

.. _AMD MicroBlaze™ V Processor:
   https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/microblaze-v.html

.. _AMD Vivado™ Design Suite:
   https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vivado.html

.. _AMD Kintex UltraScale FPGA KCU105 Evaluation Kit:
   https://www.xilinx.com/products/boards-and-kits/kcu105.html
