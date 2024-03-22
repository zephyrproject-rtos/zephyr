.. _intel_socfpga_agilex5_socdk:

Intel® Agilex™ 5 SoC FPGA Development Kit
#########################################

Overview
********

The Intel® Agilex™ 5 SoC FPGA Development Kit offers a complete design
environment that includes both hardware and software for developing
Intel® Agilex™ 5 E-Series based FPGA designs. This kit is recommended for
developing custom ARM* processor-based SoC designs and ideal for intelligent
applications at the edge, embedded and more.

Hardware
********

The Intel® Agilex™ 5 Development Kit supports the following physical features:

- Intel® Agilex™ 5 E-Series FPGA, 50K-656K LEs integrated with
  multi-core ARM processors of Dual-core A55 and Dual-core A76
- On-board 8 GB DDR5 memory
- On-board JTAG Intel FPGA Download Cable II
- QSPI flash daughtercard

Supported Features
==================
The Intel® Agilex™ 5 SoC Development Kit configuration supports the following
hardware features:

+-----------+------------+---------------------------------------------+
| Interface | Controller | Hardware Subsystem Vendor                   |
+===========+============+=============================================+
| GIC-600   | on-chip    | ARM GICv3 interrupt controller              |
+-----------+------------+---------------------------------------------+
| UART      | on-chip    | Synopsys Designware,NS16550 compatible      |
+-----------+------------+---------------------------------------------+
| ARM TIMER | on-chip    | ARM system timer                            |
+-----------+------------+---------------------------------------------+
| Reset     | on-chip    | Intel Corporation, SoCFPGA Reset controller |
+-----------+------------+---------------------------------------------+
| Clock     | on-chip    | Intel Corporation, SoCFPGA Clock controller |
+-----------+------------+---------------------------------------------+

NOTE: TODO, more details on dev kit will be updated as and when available.

The default configuration can be found in the defconfig file:
        `boards/intel/socfpga/agilex5_socdk/intel_socfpga_agilex5_socdk_defconfig`

Programming and Debugging
*************************

Zephyr Boot Flow
****************
Zephyr image will need to be loaded by Intel Arm Trusted Firmware (ATF).
ATF BL2 is the First Stage Boot Loader (FSBL) and ATF BL31 is the Run time resident firmware which
provides services like SMC (Secure monitor calls) and PSCI (Power state coordination interface).

Boot flow:
        ATF BL2 (EL3) -> ATF BL31 (EL3) -> Zephyr (EL1)

Intel Arm Trusted Firmware (ATF) can be downloaded from github:
        `altera-opensource/arm-trusted-firmware <https://github.com/altera-opensource/arm-trusted-firmware.git>`_

Flashing
========
Zephyr image can be loaded in DDR memory at address 0x80000000 from
SD Card or QSPI Flash or NAND in ATF BL2.

Debugging
=========
The Intel® Agilex™ 5 SoC Development Kit includes one JTAG connector on
board, connect it to Intel USB blaster download cables for debugging.

Zephyr applications running on the Cortex-A55/A76 core can be tested by
observing UART console output.

References
==========
`Intel® Agilex™ 5 FPGA and SoC FPGA <https://www.intel.in/content/www/in/en/products/details/fpga/agilex/5.html>`_
