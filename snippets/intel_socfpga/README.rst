.. _intel_socfpga:

Intel® Agilex™ SoC FPGA Series Snippet (intel_socfpga)
######################################################

Overview
********

This snippet streamlines configuration and device tree management for Intel Agilex
SoC FPGA series boards, enabling all peripherals for supported boards.

It specifically supports the following boards:
Intel SoC FPGA Agilex SoCDK
Intel SoC FPGA Agilex5 SoCDK

Building with the Snippet:
**************************

* Using west:

.. code-block:: console

   west build -S intel_socfpga [...]

* Using CMake:

.. code-block:: console

   cmake -S app -B build -DSNIPPET="intel_socfpga" [...]

Configuration and Device Tree Files
***********************************

The snippet applies board-specific configuration and device tree files based on the selected board:

.. list-table::
   :header-rows: 1

   * - Board
     - Device Tree Overlay File
     - Configuration File

   * - Intel SoC FPGA Agilex SoCDK
     - boards/intel_socfpga_agilex_socdk.overlay
     - boards/intel_socfpga_agilex_socdk.conf

   * - Intel SoC FPGA Agilex5 SoCDK
     - boards/intel_socfpga_agilex5_socdk.overlay
     - boards/intel_socfpga_agilex5_socdk.conf

