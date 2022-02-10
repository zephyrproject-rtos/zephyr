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
#endif
/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)
#if defined(CONFIG_SMP)
GDATA(hart_wake_flags)
GDATA(riscv_cpu_sp)
#endif

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
	/* First stage of wake up - we assume for MPFS that array has CONFIG_MP_NUM_CPUS + 1 */
	slli a1, a0, 3
	la a2, hart_wake_flags
	add a2, a2, a1
	li a1, RV_WAKE_INIT
	sd a1, 0(a2)
#endif	
#if defined(CONFIG_SOC_MPFS) && defined(CONFIG_SMP)
/*
 * For MPFS SMP operation the e51 cannot be part of the SMP group
 * of harts as it is not the same as the U54s... 
 * Just loop for ever for now.
 */
	bnez a0, not_e51
no_e51:
	/* Disable interrupts etc to try and keep things quiet */
	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	wfi
	j no_e51
	
not_e51:
#endif
#if defined(CONFIG_SOC_MPFS)
    /*
     * Temporary step, zeroise PMPs in case random values cause issues...
     *
     * MPFS has semi-random values in the registers after power up so this is
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
#endif
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
#endif
#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	la t0, z_interrupt_stacks
#if CONFIG_SMP
	li t1, CONFIG_ISR_STACK_SIZE * CONFIG_MP_NUM_CPUS
#else
	li t1, CONFIG_ISR_STACK_SIZE
#endif
	add t1, t1, t0

#if CONFIG_SMP
#ifdef CONFIG_BOARD_HART_TO_USE
    li a1, CONFIG_BOARD_HART_TO_USE
#else
    li a1, 0
#endif
    csrr a0, mhartid
    bne a0, a1, skip_stack_fill
#endif
	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
	li t2, 0xaaaaaaaa
aa_loop:
	sw t2, 0x00(t0)
	addi t0, t0, 4
	blt t0, t1, aa_loop
#endif

	/*
	 * Initially, setup stack pointer to
	 * z_interrupt_stacks[cpu] + CONFIG_ISR_STACK_SIZE
	 */
skip_stack_fill:
	la sp, z_interrupt_stacks
	li t0, CONFIG_ISR_STACK_SIZE
#if defined(CONFIG_SMP)
	csrr a0, mhartid
#if defined(CONFIG_SOC_MPFS)
	addi a0, a0, -1
#endif
	mul a1, a0, t0
	add sp, sp, a1
#endif
	add sp, sp, t0

	csrw mscratch, sp

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif

#ifdef CONFIG_BOARD_HART_TO_USE
	li a1, CONFIG_BOARD_HART_TO_USE
#else
	li a1, 0
#endif
	csrr a0, mhartid
	beq a0, a1, boot_master_core

#if CONFIG_SMP
	/* Second stage of wake up - a2 already set up */
	li a1, RV_WAKE_WAIT
	sd a1, 0(a2)
	li a0, RV_WAKE_GO
#endif	
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
#if defined(CONFIG_SOC_MPFS)
	addi a0, a0, -1
#endif
	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	call z_riscv_secondary_start
	
#else
	j loop_slave_core
#endif
boot_master_core:
	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call _PrepC
