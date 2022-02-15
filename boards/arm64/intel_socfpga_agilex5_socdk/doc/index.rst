.. _intel_socfpga_agilex5_socdk:

Intel Agilex5 SoC Development Kit
#################################

Overview
********

The Intel Agilex5 SoC Development Kit offers a complete design environment
that includes both hardware and software for developing Intel Agilex5
FPGA designs.

Hardware
********

The Intel Agilex5 SoC Development Kit supports the following physical features:

<TBD>

Supported Features
==================
The Intel Agilex5 SoC Development Kit configuration supports the following
hardware features:

+-----------+------------+--------------------------------------+
| Interface | Controller | Driver/Component                     |
+===========+============+======================================+
| GIC-600   | on-chip    | GICv3 interrupt controller           |
+-----------+------------+--------------------------------------+
| ARM TIMER | on-chip    | System Clock                         |
+-----------+------------+--------------------------------------+
| UART      | on-chip    | NS16550 compatible serial port       |
+-----------+------------+--------------------------------------+

The default configuration can be found in the defconfig file:

        ``boards/arm64/intel_socfpga_agilex5_socdk/intel_socfpga_agilex5_socdk_defconfig``

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
Zephyr image can be loaded in DDR memory at address 0x80100000 from
SD Card or QSPI Flash or Nand Flash in ATF BL2.

Debugging
=========
The Intel Agilex5 SoC Development Kit includes one JTAG connector on
board, connect it to Intel USB blaster download cables for debugging.

Zephyr applications running on the Cortex-A55/Cortex-A76 core can be tested by
observing UART console output.

References
==========
<TBD>
