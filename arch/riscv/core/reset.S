/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)

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
#if defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1)
	csrr t0, mhartid
	bnez t0, boot_slave_core
#else
	/*
	 * Zephyr UP kernel will boot master core, just halt other cores.
	 */
	csrr t0, mhartid
	bnez t0, loop_slave_core
#endif /* defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1) */

#ifdef CONFIG_FPU
	/*
	 * Enable floating-point.
	 */
	li  t0, MSTATUS_FS_INIT
	csrrs x0, mstatus, t0

	/*
	 * Floating-point rounding mode set to IEEE-754 default, and clear
	 * all exception flags.
	 */
	fscsr x0, x0
#endif

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
	li t1, CONFIG_ISR_STACK_SIZE
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
	 * z_interrupt_stacks + CONFIG_ISR_STACK_SIZE
	 */
	la sp, z_interrupt_stacks
	li t0, CONFIG_ISR_STACK_SIZE
	add sp, sp, t0

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif

	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call _PrepC

#if defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1)
boot_slave_core:
	/*
	 * If the platform has more cores than configured, these cores should goto
	 * infinite loop. (mhartid >= CONFIG_MP_NUM_CPUS)
	 */
	addi t2, t0, (-1 * CONFIG_MP_NUM_CPUS)
	bgez t2, loop_slave_core

	/*
	 * Disable interrupt before this core starts to use stack pointer (set
	 * stack pointer), because kernel will reuse interrupt stack as
	 * system stack during kernel initialization.
	 */
	csrc mstatus, MSTATUS_IEN

	/* t1 = cpu_init_boot_flag[mhartid] */
	la t1, cpu_init_boot_flag
	slli t2, t0, 2
	add t1, t1, t2

slave_core_wait:
	/* Wait until CPU0 set cpu_init_boot_flag[mhartid] by arch_start_cpu() */
	lw t2, 0(t1)
	beqz t2, slave_core_wait

	/* t1 = cpu_init_stack[mhartid] */
	la t1, cpu_init_stack
	slli t2, t0, RV_REGSHIFT
	add t1, t1, t2

	/* Initialize stack by cpu_init_stack[mhartid] */
	RV_OP_LOADREG sp, 0(t1)

	/* Set 1st argument of slave_core_cstart(): mhartid */
	mv a0, t0

	j slave_core_cstart
#endif /* defined(CONFIG_SMP) || (CONFIG_MP_NUM_CPUS > 1) */

loop_slave_core:
	wfi
	j loop_slave_core

