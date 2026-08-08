/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal AArch64 EL3 boot shim for qemu_cortex_a53_aarch32.
 *
 * QEMU's -bios entry starts the primary CPU at EL3 in AArch64. Zephyr for this
 * board is AArch32 and starts at EL1 in Supervisor (SVC) mode, so this shim
 * performs the EL3 -> AArch32-EL1 transition in place of firmware such as TF-A,
 * letting the board boot on its own.
 *
 * What the shim does, and why (per-register detail is at each write below):
 *   - Enters Zephyr in Non-secure EL1
 *   - Parks secondary CPUs
 *   - Installs no EL3 vector table
 *
 * It is assembled standalone with the SDK's aarch64-zephyr-elf toolchain (see the
 * board CMakeLists.txt), so it uses literal register values, not Zephyr headers:
 *   0x60000000  Zephyr load/entry address (matches chosen zephyr,sram)
 *   0x08000000  GICv3 distributor (GICD) base
 *   62500000    counter-timer frequency (== CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
 */

    .section .text._shim_start, "ax"
    .globl _shim_start
_shim_start:
    /* Mask all interrupts/exceptions while we bring the CPU up. */
    msr    DAIFSet, #0xf

    /* Uniprocessor only: park any secondary CPU (Aff2:Aff1:Aff0 != 0). */
    mrs    x0, mpidr_el1
    and    x0, x0, #0xffffff
    cbnz    x0, .Lpark

    /* SCTLR_EL3: RES1 bits, MMU and caches off. */
    ldr    w0, =0x30c50830 /* SCTLR_EL3_RES1 */
    msr    sctlr_el3, x0
    isb

    /* Do not trap FP/SIMD or trace accesses from lower ELs. */
    msr    cptr_el3, xzr

    /* Program the counter-timer frequency (writable only at highest EL). */
    ldr    w0, =62500000
    msr    cntfrq_el0, x0

    /* Enable system-register access to the GIC CPU interface. */
    mov    x0, #0xf /* SRE | DFB | DIB | Enable */
    msr    ICC_SRE_EL3, x0

    /*
     * Single security state: set GICD_CTLR.DS. Once DS is set it is
     * sticky, so program it here before any group configuration.
     */
    ldr    x1, =0x08000000 /* GICD base */
    ldr    w0, [x1]
    orr    w0, w0, #(1 << 6) /* GICD_CTLR.DS */
    str    w0, [x1]

    /*
     * SCR_EL3 = 0x1b1: NS=1 (lower ELs Non-secure; required for GICv3 Group-1
     * IRQ delivery under QEMU) | RES1 (bits 4,5) | SMD (bit 7) | HCE (bit 8).
     * RW (bit 10) = 0 so lower ELs execute in AArch32 state.
     */
    ldr    w0, =0x1b1
    msr    scr_el3, x0

    /* Return to Zephyr's AArch32 reset vector at EL1. */
    ldr    x0, =0x60000000
    msr    elr_el3, x0

    /*
     * SPSR for the ERET: AArch32 Supervisor (SVC) mode with A/I/F masked.
     * Value 0x1d3 = A(8) | I(7) | F(6) | mode SVC(0x13); T=0 (ARM), E=0
     * (little-endian). Must NOT reuse an AArch64 DAIF mask: bit 9 is the
     * AArch32 E (endianness) bit and would select big-endian.
     */
    mov    x0, #0x1d3
    msr    spsr_el3, x0

    isb
    eret

.Lpark:
    wfe
    b    .Lpark
