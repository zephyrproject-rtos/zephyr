.. mbv32:

AMD mbv32
#########

Overview
********

mbv32 board is based on Microblaze V design preset example from AMD Vivado™ Design Suite targeted
for "AMD Kintex UltraScale FPGA KCU105" evaluation kit. Said design preset example is build
with realtime configuration (with fast interrupt disabled). Example design system consist of
following IP blocks:

.. code-block:: console

        AMD Microblaze V processsor core FPGA IP
        AXI INTC FPGA IP
        AXI Timer FPGA IP
        AXI I2C FPGA IP
        AXI UARTLITE FPGA IP
        AXI GPIO FPGA IP
        AXI QSPI FPGA IP

Also, it consist of following memory blocks,

.. code-block:: console

        Local BRAM Memory (128 KB)
        DDR Memory (2 GB)

mbv32 realtime example design
=============================

Prebuilt mbv32 example design system is available in AMD Vivado™ Design Suite. It can be also found
at AMD Microblaze RISC-V wiki page. It consist of bitstream file needed for FPGA configuration.

Download Zephyr elf file and run application
============================================

To download the Zephyr Executable and Linkable Format .elf file, please use following west command.

.. code-block:: console

        west flash --runner xsdb --elf-file mbv32/zephyr/zephyr.elf --bitstream <path>/system.bit

References
==========

- `AMD MicroBlaze™ V Processor`_
- `AMD Vivado™ Design Suite`_
- `AMD Kintex UltraScale FPGA KCU105 Evaluation Kit`_

.. _AMD MicroBlaze™ V Processor:
   https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/microblaze-v.html

.. _AMD Vivado™ Design Suite:
   https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/vivado.html

.. _AMD Kintex UltraScale FPGA KCU105 Evaluation Kit:
   https://www.xilinx.com/products/boards-and-kits/kcu105.html
