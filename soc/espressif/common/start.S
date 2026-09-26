/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Boot entry point. The ROM enters here on a stack of its own choosing, which
 * on some layouts overlaps .bss, so this must run before any compiler-generated
 * prologue can spill to it. Early boot runs on the CPU 0 interrupt stack until
 * z_cstart() installs the kernel stack.
 */

#include <zephyr/toolchain.h>
#include <zephyr/devicetree.h>
#include <zephyr/offsets.h>
#include <zephyr/arch/riscv/csr.h>
#include <soc/soc_caps.h>

#define CSR_MTVT 0x307

GTEXT(start_riscv)
GTEXT(_vector_table)
GDATA(z_interrupt_stacks)
#if SOC_INT_CLIC_SUPPORTED
GTEXT(_mtvt_table)
#endif

GTEXT(__start)

	.section .iram1, "ax"
	.balign 4
	.global __start
	.type __start, @function

__start:
#ifdef CONFIG_RISCV_GP
	/* Must precede any access the linker could relax to gp-relative. */
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#endif

	la sp, z_interrupt_stacks
	li t0, __z_interrupt_stack_SIZEOF
	add sp, sp, t0

	csrci mstatus, MSTATUS_MIE

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
	/* Total size of all cores' IRQ stack */
	li t1, __z_interrupt_all_stacks_SIZEOF
	add t1, t1, t0

	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
	li t2, 0xaaaaaaaa
aa_loop:
	sw t2, 0x00(t0)
	addi t0, t0, 4
	blt t0, t1, aa_loop
#endif /* CONFIG_INIT_STACKS */

	la t0, _vector_table
	csrw mtvec, t0

#if SOC_INT_CLIC_SUPPORTED
	la t0, _mtvt_table
	csrw CSR_MTVT, t0
#endif

	tail start_riscv

	.size __start, . - __start
