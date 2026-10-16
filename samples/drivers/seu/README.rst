.. zephyr:code-sample:: seu
   :name: Single Event Upset (SEU)

   Detect and inject Single Event Upset (SEU) and ECC errors on Intel SoC FPGA.

Overview
********

This sample demonstrates the Intel SoC FPGA SEU driver. It registers SEU and ECC
callbacks, injects a safe SEU error, reads SEU statistics, and injects an ECC
error.

Building and Running
********************

Build for the Intel Agilex5 SoCDK:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/seu
   :board: intel_socfpga_agilex5_socdk
   :goals: build

This sample is validated on ``intel_socfpga_agilex5_socdk``.

Load the Zephyr image with ATF (for example QSPI or SD Card). On Agilex5 with
the usual ATF handoff, BL31 places Zephyr at ``0x80100000``.

Sample Output
*************

The following was captured on Agilex5 with QSPI boot:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-16170-g533ea3934fb3 ***
   Secondary CPU core 1 (MPID:0x100) is up
   Secondary CPU core 2 (MPID:0x200) is up
   Secondary CPU core 3 (MPID:0x300) is up
   SEU Test Started
   The Client No is 0x3c04a8de
   The Client No is 0x3c13eea0
   SEU Safe Error Insert Test Started
   The SEU Error Type: 1:
   The Sector Address: 5
   The Correction status: 0
   The row frame index: 0
   The bit position: 0
   SEU Safe Error Insert Test Completed
   Read SEU Statistics Test Started
   The value of t_seu_cycle : 0x1
   The value of t_seu_detect : 0x50000
   The value of t_seu_correct : 0x20000000
   The value of t_seu_inject_detect : 0xffffffff
   The value of t_sdm_seu_poll_interval : 0xbfffffff
   The value of t_sdm_seu_pin_toggle_overhead : 0xffeffdff
   Read SEU Statistics Test Completed
   Read ECC Error Test Started
   The SEU Error Type: 1:
   The Sector Address: 5
   The Correction status: 0
   The row frame index: 0
   The bit position: 0
   Read ECC Error Test Completed
   SEU Test Completed
