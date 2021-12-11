/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#if defined(CONFIG_SMP)
#include <rv_smp_defs.h>
#endif /* CONFIG_SMP */
/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)
#if defined(CONFIG_SMP)
GDATA(hart_wake_flags)
GDATA(riscv_cpu_sp)
#endif /* CONFIG_SMP */

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
    la ra, __initialize /* Keeps debugger happy so it does not follow false trails... */

#if CONFIG_SMP
    csrr a0, mhartid /* Who are we? */
	/* First stage of wake up - we assume for MPFS and other systems with monitor hart 
	 * that array has CONFIG_MP_NUM_CPUS + 1 */
	slli a1, a0, 3
	la a2, hart_wake_flags
	add a2, a2, a1
	li a1, RV_WAKE_INIT
	sd a1, 0(a2)
#if CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR
/*
 * For SMP operation the monitor hart cannot be part of the SMP group
 * of harts as it is generally not the same as the application harts... 
 * Just loop for ever for now if we get here.
 */
	li a1, CONFIG_MONITOR_HART_ID
	bne a0, a1, not_monitor_hart

no_monitor_allowed:
	/* Disable interrupts etc to try and keep things quiet */
	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	wfi
	j no_monitor_allowed
	
not_monitor_hart:
#endif	/* CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR */
#endif	/* CONFIG_SMP */

#if defined(CONFIG_CLEAR_PMP_REGS)
    /*
     * Temporary step, zeroise PMPs in case random values cause issues...
     *
     * RISC-V has semi-random values in the registers after power up so this is
     * a good idea unless you are booting from HSS etc which have already
     * setup up PMPs.
     */
    csrw pmpcfg0,0
    csrw pmpcfg2,0
    csrw pmpaddr0,0
    csrw pmpaddr1,0
    csrw pmpaddr2,0
    csrw pmpaddr3,0
    csrw pmpaddr4,0
    csrw pmpaddr5,0
    csrw pmpaddr6,0
    csrw pmpaddr7,0
    csrw pmpaddr8,0
    csrw pmpaddr9,0
    csrw pmpaddr10,0
    csrw pmpaddr11,0
    csrw pmpaddr12,0
    csrw pmpaddr13,0
    csrw pmpaddr14,0
    csrw pmpaddr15,0
#endif /* CONFIG_CLEAR_PMP_REGS */
    /*
     * This will boot master core which is lowest numbered, just halt other cores.
     *
     * Note. For systems with monitor hart we assume no supervisor supported on monitor
	 * hart so if this is selected as the hart to use we don't touch mxdeleg registers.
	 * Also, if not in master mode this is a bad thing to do...
     */
#if NO_SUPERVISOR_DELEGATION
#if CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR
	bne a0, a1, not_super_capable /* regs set up earlier... */
#endif /* CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR */
    csrw mideleg,0
    csrw medeleg,0

not_super_capable:
#endif /* NO_SUPERVISOR_DELEGATION */

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
#endif /* CONFIG_FPU */
#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
#if CONFIG_SMP
	li t1, CONFIG_ISR_STACK_SIZE * CONFIG_MP_NUM_CPUS
	add t1, t1, t0
	/* Only do this once on the startup hart */
    li a1, CONFIG_HART_TO_USE
    csrr a0, mhartid
    bne a0, a1, skip_stack_fill
#else /* CONFIG_SMP */
	li t1, CONFIG_ISR_STACK_SIZE
	add t1, t1, t0
#endif /* CONFIG_SMP */

	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
	li t2, 0xaaaaaaaa
aa_loop:
	sw t2, 0x00(t0)
	addi t0, t0, 4
	blt t0, t1, aa_loop

skip_stack_fill:
#endif /* CONFIG_INIT_STACKS */

	/*
	 * Initially, setup stack pointer to
	 * z_interrupt_stacks[cpu] + CONFIG_ISR_STACK_SIZE
	 */
	la sp, z_interrupt_stacks
	li t0, CONFIG_ISR_STACK_SIZE
#if defined(CONFIG_SMP)
	csrr a0, mhartid
#if CONFIG_SMP_BASE_CPU != 0
	addi a0, a0, -CONFIG_SMP_BASE_CPU
#endif /* CONFIG_SMP_BASE_CPU != 0 */
	mul a1, a0, t0
	add sp, sp, a1
#endif /* CONFIG_SMP */
	add sp, sp, t0

	csrw mscratch, sp

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif /* CONFIG_WDOG_INIT */

	li a1, CONFIG_HART_TO_USE
	csrr a0, mhartid
	beq a0, a1, boot_master_core

#if CONFIG_SMP
	/* Second stage of wake up - a2 already set up */
	li a1, RV_WAKE_WAIT
	sd a1, 0(a2)
	li a0, RV_WAKE_GO
#endif	/* CONFIG_SMP */
	/*
	 * Enable software interrupt
	 */
	li  t0, MIP_MSIP
	csrrs x0, mie, t0

loop_slave_core:
	wfi
#if CONFIG_SMP
	ld a1, 0(a2)
	bne a0, a1, loop_slave_core
	/*
	 * Jump into C domain assuming _PrepC has already been called.
	 */
	li a1, RV_WAKE_DONE
	sd a1, 0(a2)
	la t0, riscv_cpu_sp
	ld t0, 0(t0)
	addi sp, t0, 0
	csrr a0, mhartid

#if CONFIG_SMP_BASE_CPU != 0
	addi a0, a0, -CONFIG_SMP_BASE_CPU /* Convert to 0 - n cpu number */
#endif /* CONFIG_SMP_BASE_CPU != 0 */

	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	call z_riscv_secondary_start
	
#else	/* CONFIG_SMP */
	j loop_slave_core
#endif	/* CONFIG_SMP */

boot_master_core:
	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call _PrepC
