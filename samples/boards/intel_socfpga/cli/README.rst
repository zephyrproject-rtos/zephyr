.. zephyr:code-sample:: intel_socfpga_cli
   :name: Intel SoC FPGA CLI

   Command line interface for Intel SoC FPGA Agilex5 with UART shell
   access to on-chip peripherals.

Overview
********

A simple program that provides a command line interface for the Intel
SoC FPGA Agilex5 platform.

Building and Running
********************

This application can be built for the Agilex5 SoC development kit as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/intel_socfpga/cli
   :board: intel_socfpga_agilex5_socdk
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

   intel_socfpga_agilex5_socdk: Starting Command Line Interface...
   agilex5$
