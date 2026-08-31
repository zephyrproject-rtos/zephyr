.. _rp2xxx-cpu1-riscv:

RP2350 Hazard3 CPU1 snippet (rp2xxx-cpu1-riscv)
################################################

Overview
********

This snippet configures an RP2350 Cortex-M33 CPU0 image to load a Hazard3 CPU1
image into its private SRAM banks and launch it.

The RP2350 critical boot flags constrain which values software may write to
``ARCHSEL``. Boards provisioned with a secure boot policy that prohibits
RISC-V execution cannot use this snippet; the launcher detects that condition
and leaves CPU1 powered off.
