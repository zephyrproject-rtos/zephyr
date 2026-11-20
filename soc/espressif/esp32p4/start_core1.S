/*
 * Copyright (c) 2026 Vasilis Liaskovitis
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>
#include <soc/soc_caps.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/irq.h>

#ifndef CSR_MTVT
#define CSR_MTVT 0x307
#endif

GTEXT(arch_secondary_cpu_init)
GDATA(riscv_cpu_sp)
GDATA(riscv_cpu_boot_flag)

GTEXT(_vector_table)
#if SOC_INT_CLIC_SUPPORTED
GTEXT(_mtvt_table)
#endif

/* Must live in memory core 1 can fetch from with caches in an unknown
 * state (HP SRAM / IRAM). Core 1 comes out of reset with its caches off,
 * so the stub is kept in HP SRAM (.iram1).
 */
	.section .iram1, "ax"
	.balign 4
	.global __esp32p4_core1_start
	.type __esp32p4_core1_start, @function
__esp32p4_core1_start:
#ifdef CONFIG_RISCV_GP
	/* Must precede any access the linker could relax to gp-relative. */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	/* Stack: top of this CPU's interrupt stack, written by
	 * arch_cpu_start() in arch/riscv/core/smp.c */
	la t0, riscv_cpu_sp
	lw sp, 0x00(t0)

	csrci mstatus, MSTATUS_MIE

	la t0, _vector_table
	csrw mtvec, t0

#if SOC_INT_CLIC_SUPPORTED
	la t0, _mtvt_table
	csrw CSR_MTVT, t0
#endif

#ifdef CONFIG_FPU
	/* The Espressif boot path enters via __start, bypassing the
	 * generic riscv reset.S that initialises mstatus.FS. Enable
	 * the FPU here before any float instruction is executed.
	 */
	li t0, MSTATUS_FS_INIT
	csrs mstatus, t0
	fscsr zero
#endif

	/* mhartid (should be 1), passed to arch_secondary_cpu_init()  */
	csrr a0, mhartid
	/* Tell core 0 we're alive */
	la t0, riscv_cpu_boot_flag
	li t1, 1
	sw t1, 0x00(t0)

	tail arch_secondary_cpu_init

	.size __esp32p4_core1_start, . - __esp32p4_core1_start
