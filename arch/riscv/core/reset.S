/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/offsets.h>
#include <zephyr/arch/riscv/csr.h>
#include "asm_macros.inc"

/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(z_prep_c)
GDATA(riscv_cpu_wake_flag)
GDATA(riscv_cpu_sp)
GTEXT(arch_secondary_cpu_init)

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

#ifdef CONFIG_THREAD_LOCAL_STORAGE
/*
 * Allocate a TLS area on the current stack, initialize it via
 * arch_riscv_early_tls_stack_update(), set tp to point to it.
 */
.macro riscv_tls_init_early
	/* Save s0/s1, else they get clobbered */
	addi sp, sp, -16
	sr s0, 0(sp)
	sr s1, 8(sp)
	mv s0, sp

	call z_tls_data_size_asm
	beqz a0, tls_skip\@
	mv s1, a0
	sub sp, sp, a0

	mv a0, s0	/* arg0: stack_top */
	mv a1, s1	/* arg1: tls_size  */
	call arch_riscv_early_tls_stack_update
tls_skip\@:
	/* Restore s0/s1. sp intentionally stays below the TLS area so the
	 * runtime stack grows away from it. The 16-byte save frame above
	 * sp is abandoned.
	 */
	lr s1, 8(s0)
	lr s0, 0(s0)
.endm
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

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

	/*
	 * Initially, setup stack pointer to
	 * z_interrupt_stacks + __z_interrupt_stack_SIZEOF
	 */
	la sp, z_interrupt_stacks
	li t0, __z_interrupt_stack_SIZEOF
	add sp, sp, t0

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/*
	 * Initialize TLS before the first C function call. Compiler
	 * inserts stack canary checks into C functions, and the canary
	 * value is read from a TLS variable via tp. Without this early
	 * init, tp is not set up and any C function called during early
	 * boot (e.g. z_prep_c) would read the canary from an arbitrary
	 * memory location.
	 */
	riscv_tls_init_early
#endif

#ifdef CONFIG_RISCV_SMRNMI_ENABLE_NMI_DELIVERY
	csrs CSR_MNSTATUS, MNSTATUS_NMIE
#endif

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif

	/*
	 * Jump into C domain. z_prep_c zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call z_prep_c

boot_secondary_core:
#if CONFIG_MP_MAX_NUM_CPUS > 1
	la t0, riscv_cpu_wake_flag
	li t1, -1
	sr t1, 0(t0)
	la t0, riscv_cpu_boot_flag
	sr zero, 0(t0)

wait_secondary_wake_flag:
	la t0, riscv_cpu_wake_flag
	lr t0, 0(t0)
	bne a0, t0, wait_secondary_wake_flag

	/* Set up stack */
	la t0, riscv_cpu_sp
	lr sp, 0(t0)

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Initialize tp early so TLS-based stack canaries work on secondary CPUs */
	mv s2, a0		/* save hartid across TLS init calls */
	riscv_tls_init_early
	mv a0, s2		/* restore hartid */
#endif

	la t0, riscv_cpu_boot_flag
	li t1, 1
	sr t1, 0(t0)
	j arch_secondary_cpu_init
#else
	j loop_unconfigured_cores
#endif

loop_unconfigured_cores:
	wfi
	j loop_unconfigured_cores
