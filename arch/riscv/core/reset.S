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
#include <zephyr/arch/riscv/irq.h>
#include "asm_macros.inc"

#ifdef CONFIG_RISCV_S_MODE
/* mcounteren bits */
#define MCOUNTEREN_CY  BIT(0)  /* cycle counter */
#define MCOUNTEREN_TM  BIT(1)  /* time counter */
#define MCOUNTEREN_IR  BIT(2)  /* instret counter */
#endif

/* exports */
GTEXT(__initialize)
GTEXT(__reset)

/* imports */
GTEXT(z_prep_c)
GDATA(riscv_cpu_wake_flag)
GDATA(riscv_cpu_sp)
GTEXT(arch_secondary_cpu_init)
#ifdef CONFIG_RISCV_S_MODE
GTEXT(__mmode_sbi_handler)
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

#ifdef CONFIG_RISCV_SMRNMI_ENABLE_NMI_DELIVERY
	csrs CSR_MNSTATUS, MNSTATUS_NMIE
#endif

#ifdef CONFIG_WDOG_INIT
	call _WdogInit
#endif

#ifdef CONFIG_RISCV_S_MODE
	/* Set M-mode trap vector to our SBI handler */
	la t0, __mmode_sbi_handler
	csrw mtvec, t0

	/* Delegate all exceptions to S-mode except S-mode ecall and
	 * M-mode ecall, which stay in M-mode to be handled by our SBI.
	 */
	li t0, ~((1 << RISCV_EXC_ECALLS) | (1 << RISCV_EXC_ECALLM))
	csrw medeleg, t0

	/* Delegate supervisor software, timer and external interrupts
	 * to S-mode.
	 */
	li t0, (MIP_SSIP | MIP_STIP | MIP_SEIP)
	csrw mideleg, t0

	/* PMP entry 0: NAPOT, RWX, covers all memory so S-mode can
	* access everything
	*/
	li t0, -1
	csrw pmpaddr0, t0
	li t0, (PMP_NAPOT | PMP_R | PMP_W | PMP_X)
	csrw pmpcfg0, t0

	/* Initialize mscratch to M-mode stack top */
	la t0, m_mode_stack_top
	csrw mscratch, t0

	/* Allow S-mode to read cycle, time and instret counters */
	li t0, (MCOUNTEREN_CY | MCOUNTEREN_TM | MCOUNTEREN_IR)
	csrw mcounteren, t0

	/* Enable M-mode timer interrupt so MTIP fires and is forwarded to S-mode as STIP */
	li t0, MIP_MTIP
	csrs mie, t0

	/* Set MPP=S-mode in mstatus then mret into S-mode */
	li t0, MSTATUS_MPP
	csrc mstatus, t0
	li t0, MSTATUS_MPP_S
	csrs mstatus, t0
	la t0, s_mode_entry
	csrw mepc, t0
	mret

s_mode_entry:
#endif /* CONFIG_RISCV_S_MODE */

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
