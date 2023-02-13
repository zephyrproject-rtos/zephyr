/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets.h>
#include "asm_macros.inc"

/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)
GTEXT(riscv_cpu_wake_flag)
GTEXT(riscv_cpu_sp)
GTEXT(z_riscv_secondary_cpu_init)

#if CONFIG_INCLUDE_RESET_VECTOR
SECTION_FUNC(reset, __reset)
	/*
	 * jump to __initialize
	 * use call opcode in case __initialize is far away.
	 * This will be dependent on linker.ld configuration.
	 */
	call __initialize
#endif /* CONFIG_INCLUDE_RESET_VECTOR */

/* use ABI name of registers for the sake of simplicity */

/*
 * Remainder of asm-land initialization code before we can jump into
 * the C domain
 */
SECTION_FUNC(TEXT, __initialize)
	csrr a0, mhartid
	li t0, CONFIG_RV_BOOT_HART
	beq a0, t0, boot_first_core
	j boot_secondary_core

boot_first_core:

#ifdef CONFIG_FPU
	/*
	 * Enable floating-point.
	 */
	li t0, MSTATUS_FS_INIT
	csrs mstatus, t0

	/*
	 * Floating-point rounding mode set to IEEE-754 default, and clear
	 * all exception flags.
	 */
	fscsr zero
#endif

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
	li t1, __z_interrupt_stack_SIZEOF
	add t1, t1, t0

	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
	li t2, 0xaaaaaaaa
aa_loop:
	sw t2, 0x00(t0)
	addi t0, t0, 4
	blt t0, t1, aa_loop
#endif

	/*
	 * Initially, setup stack pointer to
	 * z_interrupt_stacks + __z_interrupt_stack_SIZEOF
	 */
	la sp, z_interrupt_stacks
	li t0, __z_interrupt_stack_SIZEOF
	add sp, sp, t0

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif

	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call _PrepC

boot_secondary_core:
#if CONFIG_MP_MAX_NUM_CPUS > 1
	la t0, riscv_cpu_wake_flag
	lr t0, 0(t0)
	bne a0, t0, boot_secondary_core

	/* Set up stack */
	la t0, riscv_cpu_sp
	lr sp, 0(t0)

	la t0, riscv_cpu_wake_flag
	sr zero, 0(t0)
	j z_riscv_secondary_cpu_init
#else
	j loop_unconfigured_cores
#endif

loop_unconfigured_cores:
	wfi
	j loop_unconfigured_cores
