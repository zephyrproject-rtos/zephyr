.. zephyr:code-sample:: riscv-aia-uart-echo
   :name: RISC-V AIA UART echo

   Echo UART input using RISC-V AIA interrupt delivery through APLIC and IMSIC.

Overview
********

This sample demonstrates RISC-V Advanced Interrupt Architecture (AIA) interrupt
delivery using UART interrupts.

The interrupt flow is:

* UART
* APLIC
* MSI
* IMSIC
* CPU

The sample configures APLIC source routing, routes the UART interrupt through an
MSI to the IMSIC, and echoes typed characters from the UART interrupt handler.

Requirements
************

This sample is intended to run on the
``qemu_riscv32/qemu_virt_riscv32/aia-imsic`` board, which provides APLIC and
IMSIC interrupt controllers in MSI delivery mode.

Configuration
*************

The sample is configured for:

* Board: ``qemu_riscv32/qemu_virt_riscv32/aia-imsic``
* UART IRQ: APLIC source 10
* Target EIID: 32
* Mode: edge-triggered MSI delivery

Building and Running
********************

Build and run the sample for the AIA IMSIC QEMU RISC-V board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/interrupt_controller/intc_riscv_aia
   :host-os: unix
   :board: qemu_riscv32/qemu_virt_riscv32/aia-imsic
   :goals: run
   :compact:

Type characters in the QEMU console to see them echoed. Press Ctrl+A, X to
exit QEMU.
