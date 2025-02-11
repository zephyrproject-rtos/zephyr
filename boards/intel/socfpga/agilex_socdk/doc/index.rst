.. _intel_socfpga_agilex_socdk:

Intel Agilex SoC Development Kit
#################################

Overview
********

The Intel Agilex SoC Development Kit offers a complete design environment
that includes both hardware and software for developing Intel Agilex
F-Series FPGA designs. This kit is recommended for developing custom
Arm* processor-based SoC designs and evaluating transceiver performance.

Hardware
********

The Intel Agilex SoC Development Kit supports the following physical features:

- Intel Agilex F-Series FPGA, 1400 KLE, 2486A package integrate the
  quad-core Arm Cortex-A53 processor
- On-board 8 GB DDR4 memory
- On-board JTAG Intel FPGA Download Cable II
- QSPI flash daughtercard
- HPS OOBE daughtercard with UART and SD Card support

Supported Features
==================
The Intel Agilex SoC Development Kit configuration supports the following
hardware features:

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| GIC-400   | on-chip    | GICv2 interrupt controller           |
+-----------+------------+--------------------------------------+
| ARM TIMER | on-chip    | System Clock                         |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | NS16550 compatible serial port       |
+-----------+------------+--------------------------------------+

Other hardware features have not been enabled yet for this board.

The default configuration can be found in
:zephyr_file:`boards/intel/socfpga/agilex_socdk/intel_socfpga_agilex_socdk_defconfig`

Programming and Debugging
*************************

Boot Flow
=========
Zephyr image will need to be loaded by Intel Arm Trusted Firmware (ATF).
ATF BL2 is first stage boot loader (FSBL) and ATF BL31 is second stage
boot loader (SSBL).

Zephyr boot flow:

        ATF BL2 (EL3) -> ATF BL31 (EL3) -> Zephyr (EL2->EL1)

Intel Arm Trusted Firmware (ATF) can be downloaded from github:

        `altera-opensource/arm-trusted-firmware <https://github.com/altera-opensource/arm-trusted-firmware.git>`_

Flashing
========
Zephyr image can be loaded in DDR memory at address 0x10000000 from
SD Card or QSPI Flash in ATF BL2.

Debugging
=========
The Intel Agilex SoC Development Kit includes one JTAG connector on
board, connect it to Intel USB blaster download cables for debugging.

Zephyr applications running on the Cortex-A53 core can be tested by
observing UART console output.

References
==========
`Intel Agilex Transceiver-SoC Development Kit <https://www.intel.com/content/www/us/en/programmable/products/boards_and_kits/dev-kits/altera/kit-agf-si.html>`_
