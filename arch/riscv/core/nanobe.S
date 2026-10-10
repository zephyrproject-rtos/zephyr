/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V nanobe context switch and ISR injection
 *
 * Nanobes run in thread mode (machine mode, interrupt level 0) on the stack
 * of the nanobe owner thread. A context switch only saves the callee-saved
 * registers, the return address and the injection guard of the outgoing
 * context on its own stack, and swaps the stack pointer. The frame layout
 * must match struct nanobe_frame in arch/riscv/core/nanobe_thread.c:
 *
 *   high address:  s11 .. s2, 2 words padding   (not on RV32E)
 *                  s1, s0
 *                  ra (resume pc)
 *   saved sp ->    injection guard
 *
 * The injection syringe returns to the interrupted code using mret, which
 * restores all the registers and the interrupt enable state of the
 * interrupted context atomically. With CONFIG_NANOBE_HW_STACKING (Nordic
 * VPR), mret also unstacks the hardware interrupt stack frame, hence the
 * syringe stacks its frame in the VPR hardware stacking layout.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/csr.h>
#include "asm_macros.inc"

/* Nanobe context frame, in registers */
#define NB_IGRD 0
#define NB_RA   1
#define NB_S0   2
#define NB_S1   3
#define NB_S2   4
#define NB_S3   5
#define NB_S4   6
#define NB_S5   7
#define NB_S6   8
#define NB_S7   9
#define NB_S8   10
#define NB_S9   11
#define NB_S10  12
#define NB_S11  13
#if defined(CONFIG_RISCV_ISA_RV32E)
#define NB_REGS 4
#else
#define NB_REGS 16
#endif

#define NB_OFF(reg) ((NB_##reg) * RV_REGSIZE)
#define NB_SIZE     (NB_REGS * RV_REGSIZE)

#define DO_NANOBE_CALLEE_SAVED(op) \
	RV_E(	op ra, NB_OFF(RA)(sp)	);\
	RV_E(	op s0, NB_OFF(S0)(sp)	);\
	RV_E(	op s1, NB_OFF(S1)(sp)	);\
	RV_I(	op s2, NB_OFF(S2)(sp)	);\
	RV_I(	op s3, NB_OFF(S3)(sp)	);\
	RV_I(	op s4, NB_OFF(S4)(sp)	);\
	RV_I(	op s5, NB_OFF(S5)(sp)	);\
	RV_I(	op s6, NB_OFF(S6)(sp)	);\
	RV_I(	op s7, NB_OFF(S7)(sp)	);\
	RV_I(	op s8, NB_OFF(S8)(sp)	);\
	RV_I(	op s9, NB_OFF(S9)(sp)	);\
	RV_I(	op s10, NB_OFF(S10)(sp)	);\
	RV_I(	op s11, NB_OFF(S11)(sp)	)

/* mstatus.MIE to mstatus.MPIE */
#define MSTATUS_MIE_TO_MPIE_SHIFT 4

/* CLIC mode mcause previous interrupt level, mcause.MPIL[23:16] */
#define CLIC_MCAUSE_MPIL_MASK (0xff << 16)

GTEXT(nanobe_switch)
GTEXT(z_nanobe_entry)
GTEXT(z_nanobe_exit)
GTEXT(z_nanobe_syringe)

GDATA(z_nanobe_sgrd)
GDATA(z_nanobe_igrd)
GDATA(z_nanobe_itrg)
GDATA(z_nanobe_iret)
GDATA(z_nanobe_ical)

/*
 * void nanobe_switch(void *next_sp, void **curr_sp);
 */
SECTION_FUNC(TEXT, nanobe_switch)
	/* save callee-saved registers of the current context */
	addi sp, sp, -NB_SIZE
	DO_NANOBE_CALLEE_SAVED(sr)

	/* save the injection guard of the current context and block
	 * injections while the stack is being swapped.
	 */
	la t0, z_nanobe_igrd
	lbu t1, 0(t0)
	sr t1, NB_OFF(IGRD)(sp)
	li t1, 1
	sb t1, 0(t0)

	/* swap the stack */
	sr sp, 0(a1)
	mv sp, a0

	/* restore the injection guard of the next context */
	lr t1, NB_OFF(IGRD)(sp)
	sb t1, 0(t0)

	/* release the nanobe scheduler lock */
	la t0, z_nanobe_sgrd
	sb zero, 0(t0)

	/* restore callee-saved registers of the next context */
	DO_NANOBE_CALLEE_SAVED(lr)
	addi sp, sp, NB_SIZE
	ret

/*
 * Nanobe entry trampoline, first resumed by nanobe_switch() with the nanobe
 * entry function in s0 and its argument in s1.
 */
SECTION_FUNC(TEXT, z_nanobe_entry)
	mv a0, s1
	jalr ra, s0, 0
	/* nanobe entry function must not return */
	call z_nanobe_exit

#if defined(CONFIG_NANOBE_HW_STACKING)
/*
 * Nordic VPR hardware interrupt stack frame (32-bit bus), unstacked by mret.
 * Checked against struct arch_esf in nanobe_thread.c.
 */
#define SY_RA     0
#define SY_T2     4
#define SY_T1     8
#define SY_T0     12
#define SY_A5     16
#define SY_A4     20
#define SY_A3     24
#define SY_A2     28
#define SY_A1     32
#define SY_A0     36
#define SY_MCAUSE 40
#define SY_MEPC   44
#define SY_SIZE   48

/* CLIC mode mcause, MPP and MPIE alias mstatus */
#define SY_MCAUSE_MPIE_SHIFT 24 /* mstatus.MIE[3] to mcause.MPIE[27] */
#define SY_MCAUSE_MPP_M      (0x3 << 28)

#define DO_SYRINGE_CALLER_SAVED(op) \
	op ra, SY_RA(sp)	;\
	op t2, SY_T2(sp)	;\
	op t1, SY_T1(sp)	;\
	op t0, SY_T0(sp)	;\
	op a5, SY_A5(sp)	;\
	op a4, SY_A4(sp)	;\
	op a3, SY_A3(sp)	;\
	op a2, SY_A2(sp)	;\
	op a1, SY_A1(sp)	;\
	op a0, SY_A0(sp)

#else /* !CONFIG_NANOBE_HW_STACKING */

/* Syringe frame, in registers */
#define SY_IRET 0
#define SY_RA   1
#define SY_T0   2
#define SY_T1   3
#define SY_T2   4
#define SY_A0   5
#define SY_A1   6
#define SY_A2   7
#define SY_A3   8
#define SY_A4   9
#define SY_A5   10
#define SY_T3   11
#define SY_T4   12
#define SY_T5   13
#define SY_T6   14
#define SY_A6   15
#define SY_A7   16
#if defined(CONFIG_RISCV_ISA_RV32E)
#define SY_REGS 12
#else
#define SY_REGS 20
#endif

#define SY_OFF(reg) ((SY_##reg) * RV_REGSIZE)
#define SY_SIZE     (SY_REGS * RV_REGSIZE)

#define DO_SYRINGE_CALLER_SAVED(op) \
	RV_E(	op ra, SY_OFF(RA)(sp)	);\
	RV_E(	op t0, SY_OFF(T0)(sp)	);\
	RV_E(	op t1, SY_OFF(T1)(sp)	);\
	RV_E(	op t2, SY_OFF(T2)(sp)	);\
	RV_E(	op a0, SY_OFF(A0)(sp)	);\
	RV_E(	op a1, SY_OFF(A1)(sp)	);\
	RV_E(	op a2, SY_OFF(A2)(sp)	);\
	RV_E(	op a3, SY_OFF(A3)(sp)	);\
	RV_E(	op a4, SY_OFF(A4)(sp)	);\
	RV_E(	op a5, SY_OFF(A5)(sp)	);\
	RV_I(	op t3, SY_OFF(T3)(sp)	);\
	RV_I(	op t4, SY_OFF(T4)(sp)	);\
	RV_I(	op t5, SY_OFF(T5)(sp)	);\
	RV_I(	op t6, SY_OFF(T6)(sp)	);\
	RV_I(	op a6, SY_OFF(A6)(sp)	);\
	RV_I(	op a7, SY_OFF(A7)(sp)	)

#endif /* CONFIG_NANOBE_HW_STACKING */

/*
 * Injection syringe.
 *
 * nanobe_isr_inject() redirects the exception return address of the
 * interrupted thread here. Entered by mret, executes in thread mode with the
 * interrupted context's registers and interrupt enable state, calls the
 * injected callee preserving all the caller-saved registers and resumes the
 * interrupted code using mret.
 */
SECTION_FUNC(TEXT, z_nanobe_syringe)
	addi sp, sp, -SY_SIZE
#if defined(CONFIG_NANOBE_HW_STACKING)
	DO_SYRINGE_CALLER_SAVED(sw)

	/* copy the interrupted return address before the callee may switch
	 * to a context that is injected.
	 */
	la t0, z_nanobe_iret
	lw t0, 0(t0)
	sw t0, SY_MEPC(sp)
#else
	DO_SYRINGE_CALLER_SAVED(sr)

	/* copy the interrupted return address before the callee may switch
	 * to a context that is injected.
	 */
	la t0, z_nanobe_iret
	lr t0, 0(t0)
	sr t0, SY_OFF(IRET)(sp)
#endif /* CONFIG_NANOBE_HW_STACKING */

1:
	la t0, z_nanobe_ical
	lr t0, 0(t0)
	jalr ra, t0, 0

	/* injection done, re-run if re-injected meanwhile */
	la t0, z_nanobe_igrd
	sb zero, 0(t0)
	la t1, z_nanobe_itrg
	lbu t2, 0(t1)
	beqz t2, 2f
	sb zero, 0(t1)
	li t2, 1
	sb t2, 0(t0)
	j 1b

2:
	/* disable interrupts until mret, which restores the current interrupt
	 * enable state of this thread, i.e. the one of the interrupted code.
	 */
	csrrci t0, mstatus, MSTATUS_IEN
	andi t0, t0, MSTATUS_IEN

#if defined(CONFIG_NANOBE_HW_STACKING)
	/* mcause: MPP = M, MPIE = MIE, MPIL = 0 (thread level), mepc is the
	 * interrupted return address without the stack alignment flag. mret
	 * unstacks the frame in hardware.
	 */
	slli t0, t0, SY_MCAUSE_MPIE_SHIFT
	li t1, SY_MCAUSE_MPP_M
	or t0, t0, t1
	sw t0, SY_MCAUSE(sp)
	csrw mcause, t0
	lw t1, SY_MEPC(sp)
	csrw mepc, t1
	mret
#else
	/* mstatus: MPP = M, MPIE = MIE */
	slli t0, t0, MSTATUS_MIE_TO_MPIE_SHIFT
	li t1, MSTATUS_MPIE
	csrc mstatus, t1
	li t1, MSTATUS_MPP
	or t0, t0, t1
	csrs mstatus, t0

#if defined(CONFIG_RISCV_HAS_CLIC)
	/* return to thread level, mcause.MPIL = 0 */
	li t0, CLIC_MCAUSE_MPIL_MASK
	csrc mcause, t0
#endif /* CONFIG_RISCV_HAS_CLIC */

	lr t0, SY_OFF(IRET)(sp)
	csrw mepc, t0

	DO_SYRINGE_CALLER_SAVED(lr)
	addi sp, sp, SY_SIZE
	mret
#endif /* CONFIG_NANOBE_HW_STACKING */
