.. zephyr:code-sample:: riscv-aia-smp-uart-echo
   :name: RISC-V AIA SMP UART echo

   Echo UART input using RISC-V AIA interrupt delivery on an SMP system.

Overview
********

This sample demonstrates RISC-V Advanced Interrupt Architecture (AIA) interrupt
delivery with multiple harts in an SMP configuration.

The sample demonstrates:

* Multi-hart boot with per-hart IMSIC initialization
* APLIC MSI routing to specific harts
* Software MSI injection with GENMSI targeting individual harts
* Dynamic interrupt routing between harts

IMSIC EIID enables are local to each hart. The sample pins an initialization
thread to hart 1 so both IMSIC files enable the UART EIID before the APLIC
routing tests target either hart.

Requirements
************

This sample is intended to run on the
``qemu_riscv32/qemu_virt_riscv32/aia-imsic/smp`` board, which provides APLIC
and IMSIC interrupt controllers in MSI delivery mode with two harts.

Configuration
*************

The sample is configured for:

* Board: ``qemu_riscv32/qemu_virt_riscv32/aia-imsic/smp``
* CPUs: 2 harts
* IMSIC: one interrupt file per hart at ``0x24000000`` and ``0x24001000``
* APLIC: routes interrupts to either hart dynamically

Building and Running
********************

Build and run the sample for the AIA IMSIC SMP QEMU RISC-V board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/interrupt_controller/intc_riscv_aia_smp
   :host-os: unix
   :board: qemu_riscv32/qemu_virt_riscv32/aia-imsic/smp
   :goals: run
   :compact:
