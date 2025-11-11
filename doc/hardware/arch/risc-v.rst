Zephyr support status on RISC-V processors
##########################################

Overview
********

This page describes current state of Zephyr for RISC-V processors.
Currently, there's support for some boards, as well as Qemu support
and support for some FPGA implementations such as neorv32 and
litex_vexriscv.

Zephyr support includes PMP, :ref:`user mode<usermode_api>`, several
ISA extensions as well as :ref:`semihosting<semihost_guide>`.

User mode and PMP support
**************************

When the platform has Physical Memory Protection (PMP) support, enabling
it on Zephyr allows user space support and stack protection to be
selected.

ISA extensions
**************

It's possible to set in Zephyr which ISA extensions (RV32/64I(E)MAFD(G)QC)
are available on a given platform, by setting the appropriate ``CONFIG_RISCV_ISA_*``
kconfig. Look at :file:`arch/riscv/Kconfig.isa` for more information.

Note that Zephyr SDK toolchain support may not be defined for all
combinations.

SMP support
***********

SMP is supported on RISC-V for both QEMU-virtualized and hardware-based
platforms. In order to test the SMP support, one can use
:zephyr:board:`qemu_riscv32` or :zephyr:board:`qemu_riscv64` for QEMU-based
platforms, or :zephyr:board:`beaglev_fire` or :zephyr:board:`mpfs_icicle` for
hardware-based platforms.
