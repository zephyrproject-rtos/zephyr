/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include "asm_macros.inc"
#include <rv_smp_defs.h>

/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(_PrepC)
GDATA(hart_wake_flags)
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
    csrr a0, mhartid /* Who are we? */
	/* First stage of wake up - we assume for MPFS and other systems with monitor hart
	 * that array has CONFIG_MP_NUM_CPUS + 1 */
	slli a1, a0, RV_REGSHIFT
	la a2, hart_wake_flags
	add a2, a2, a1
	li a1, RV_WAKE_INIT
	sr a1, 0(a2)
#if CONFIG_HAS_MONITOR_HART
/*
 * For SMP operation the monitor hart cannot be part of the SMP group
 * of harts as it is generally not the same as the application harts...
 * Just loop for ever for now if we get here.
 */
	li a1, CONFIG_MONITOR_HART_ID
	bne a0, a1, not_monitor_hart

	/*
	 * Disable interrupts etc to try and keep things quiet.
	 * But first assign the first isr stack to us for safety,
	 * If monitor hart is not hart id 0, this will need changing...
	 */
	la sp, z_interrupt_stacks
	li t0, CONFIG_ISR_STACK_SIZE
	add sp, sp, t0
no_monitor_loop:
	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	wfi
	j no_monitor_loop

not_monitor_hart:
#endif	/* CONFIG_HAS_MONITOR_HART */
    /*
     * This will boot primary core which is usually thelowest numbered, just halt
	 * other cores.
     *
     * Note. For systems with monitor hart we assume no supervisor supported on monitor
	 * hart so if this is selected as the hart to use we don't touch mxdeleg registers.
	 * Also, if not in master mode this is a bad thing to do...
     */
#if CONFIG_NO_SUPERVISOR_DELEGATION
#if CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR
	beq a0, a1, not_super_capable /* regs set up earlier... */
#endif /* CONFIG_HAS_MONITOR_HART && !CONFIG_MONITOR_HART_SUPERVISOR */
    csrw mideleg,0
    csrw medeleg,0

not_super_capable:
#endif /* CONFIG_NO_SUPERVISOR_DELEGATION */

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
	/* Get the larger of */
#if (CONFIG_MP_TOTAL_NUM_CPUS > CONFIG_MP_NUM_CPUS)
	li t1, CONFIG_ISR_STACK_SIZE * CONFIG_MP_TOTAL_NUM_CPUS
#else
	li t1, CONFIG_ISR_STACK_SIZE * CONFIG_MP_NUM_CPUS
#endif
	add t1, t1, t0
	/* Only do this once on the startup hart - slightly risky as we are relying
	 * on other harts not using the stack until we allow them to wake...*/
    li a1, CONFIG_SMP_BASE_CPU
    csrr a0, mhartid
    bne a0, a1, skip_stack_fill

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
*
* We always allocate these stacks based on the real hart id as we need to
* account for all harts when allocating an initial stack even if
* they are not going to run Zephyr...
*/

li t0, CONFIG_ISR_STACK_SIZE
csrr a0, mhartid
la a1, z_interrupt_stacks
shiftmul_add a1, a0, CONFIG_ISR_STACK_SIZE
add sp, a1, t0
csrw mscratch, sp /* mscratch contains address of interrupt stack now */


#ifdef CONFIG_WDOG_INIT
call _WdogInit
#endif

	li a1, CONFIG_SMP_BASE_CPU
	csrr a0, mhartid
	beq a0, a1, boot_primary_core

	/* Second stage of wake up - a2 already set up */
	li a1, RV_WAKE_WAIT
	sr a1, 0(a2)
	li a0, RV_WAKE_GO
	/*
	 * Enable software interrupt
	 */
	li  t0, MIP_MSIP
	csrrs x0, mie, t0
	/*
	 * Loop here with wfi to try and keep the bus quiet if multiple
	 * harts are spinning on same region.
	 */
loop_secondary_core:
	wfi

	lr a1, 0(a2)
	bne a0, a1, loop_secondary_core
	/*
	 * Jump into C domain assuming _PrepC has already been called.
	 */
	li a1, RV_WAKE_DONE
	sr a1, 0(a2)
	la t0, riscv_cpu_sp
	lr t0, 0(t0)
	addi sp, t0, 0
	csrr a0, mhartid

#if CONFIG_SMP_BASE_CPU != 0
	addi a0, a0, -CONFIG_SMP_BASE_CPU /* Convert to 0 - n cpu number */
#endif /* CONFIG_SMP_BASE_CPU != 0 */

	csrw mstatus, zero
    csrw mie, zero
    csrw mip, zero
	call z_riscv_secondary_cpu_init

boot_primary_core:
	/*
	 * Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters kernel z_cstart
	 */
	call _PrepC

