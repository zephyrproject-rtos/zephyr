/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)

SECTION_FUNC(vectors, __start)
	.option norvc;

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to __irq_wrapper.
	 */
	la t0, __irq_wrapper
	csrw mtvec, t0

	/* Read the hart ID */
	csrr t0, mhartid

#ifndef CONFIG_SMP
	/* If we're not running with SMP enabled, then make sure any other harts
	 * are parked forever */
	bnez t0, park_hart_forever
#else /* CONFIG_SMP */
	/* If the hart ID is >= the configured number of CPUs, park forever
	 *
	 * Greater than or equal to because if CONFIG_NUM_CPUS == 3, then the
	 * last hart will be hart 2. */
	li t1, CONFIG_MP_NUM_CPUS
	bgeu t0, t1, park_hart_forever

	/* If we're not hart 0, wait for _arch_start_cpu to start our hart */
	bnez t0, secondary_harts
#endif /* CONFIG_SMP */

	/* Hart 0 jumps to __initialize */
	tail __initialize

park_hart_forever:
	wfi
	j park_hart_forever

#ifdef CONFIG_SMP
secondary_harts:
	/* multiply the hartid by 4 */
	slli t0, t0, 2

	/* load the address of the go flag */
	la t1, __init_riscv_smp_go
	add t1, t1, t0

secondary_harts_wait:
	/* load the go flag and see if we've been signaled */
	lw t2, (t1)
	bnez t2, secondary_harts_go
	j secondary_harts_wait

secondary_harts_go:
	/* load our start point and jump into it */

	/* load the cpu context into mscratch */
	la t2, __init_riscv_smp_mscratch
	add t2, t2, t0
	lw t2, 0(t2)
	csrw mscratch, t2

	/* load entry function into t1 */
	la t1, __init_riscv_smp_entry
	add t1, t1, t0
	lw t1, 0(t1)

	/* load the stack pointer */
	la sp, __init_riscv_smp_stacks
	add sp, sp, t0
	lw sp, 0(sp)

	/* load the irq_unlock key into a0 */
	la a0, __init_riscv_smp_keys
	add a0, a0, t0
	lw a0, 0(a0)

	/* load the start flag into a1 */
	la a1, __init_riscv_smp_start_flags
	add a1, a1, t0
	lw a1, 0(a1)

	/* call the entry function */
	jr t1
#endif /* CONFIG_SMP */
