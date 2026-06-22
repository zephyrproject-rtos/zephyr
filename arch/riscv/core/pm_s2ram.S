/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/offsets.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/riscv/csr.h>
#include "asm_macros.inc"

/* Byte sizes used to lay out the on-stack save area. asm_macros.inc
 * provides the load/store mnemonics (lr/sr, flr/fsr) but the offsets
 * within the stack frame still need to be computed by hand.
 */
#ifdef CONFIG_64BIT
#define REGBYTES 8
#else
#define REGBYTES 4
#endif

#ifdef CONFIG_FPU
#ifdef CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#define FREGBYTES 8
#else
#define FREGBYTES 4
#endif
#endif

/* CLIC vector-table-base CSR. */
#ifndef CSR_MTVT
#define CSR_MTVT 0x307
#endif

/* Stack layout: ra, gp, tp, s0-s11 (s0-s1 only on RV32E), optional
 * fs0-fs11 + fcsr. Padded up to the 16-byte sp alignment the RISC-V
 * psABI requires at function call boundaries.
 */
#ifdef CONFIG_RISCV_ISA_RV32E
#define GPR_AREA_BYTES   (5 * REGBYTES)
#else
#define GPR_AREA_BYTES   (15 * REGBYTES)
#endif

#ifdef CONFIG_FPU
#define FPR_AREA_OFF  GPR_AREA_BYTES
#define FCSR_OFF   (FPR_AREA_OFF + 12 * FREGBYTES)
#define STACK_RAW_BYTES  (FCSR_OFF + REGBYTES)
#else
#define STACK_RAW_BYTES  GPR_AREA_BYTES
#endif

#define STACK_FRAME_BYTES ((STACK_RAW_BYTES + 15) & ~15)

/* Save or load callee-saved GPRs at fixed offsets in the current stack
 * frame. Single macro parameterised by op = sr (store) or lr (load),
 * mirroring DO_CALLEE_SAVED in switch.S. RV_E / RV_I expand to op on
 * RV32E / RV32I respectively.
 */
#define DO_STACK_GPRS(op) \
	RV_E(	op   ra, ( 0 * REGBYTES)(sp)	);\
	RV_E(	op   gp, ( 1 * REGBYTES)(sp)	);\
	RV_E(	op   tp, ( 2 * REGBYTES)(sp)	);\
	RV_E(	op   s0, ( 3 * REGBYTES)(sp)	);\
	RV_E(	op   s1, ( 4 * REGBYTES)(sp)	);\
	RV_I(	op   s2, ( 5 * REGBYTES)(sp)	);\
	RV_I(	op   s3, ( 6 * REGBYTES)(sp)	);\
	RV_I(	op   s4, ( 7 * REGBYTES)(sp)	);\
	RV_I(	op   s5, ( 8 * REGBYTES)(sp)	);\
	RV_I(	op   s6, ( 9 * REGBYTES)(sp)	);\
	RV_I(	op   s7, (10 * REGBYTES)(sp)	);\
	RV_I(	op   s8, (11 * REGBYTES)(sp)	);\
	RV_I(	op   s9, (12 * REGBYTES)(sp)	);\
	RV_I(	op  s10, (13 * REGBYTES)(sp)	);\
	RV_I(	op  s11, (14 * REGBYTES)(sp)	)

/* FP save/restore is gated at runtime on mstatus.FS != Off. The slot
 * is always reserved in the stack frame so the offsets stay constant;
 * only the load/store work is skipped when there's no FP state.
 */
#ifdef CONFIG_FPU
#define DO_STACK_FPRS(op) \
	op fs0,  (FPR_AREA_OFF +  0 * FREGBYTES)(sp); \
	op fs1,  (FPR_AREA_OFF +  1 * FREGBYTES)(sp); \
	op fs2,  (FPR_AREA_OFF +  2 * FREGBYTES)(sp); \
	op fs3,  (FPR_AREA_OFF +  3 * FREGBYTES)(sp); \
	op fs4,  (FPR_AREA_OFF +  4 * FREGBYTES)(sp); \
	op fs5,  (FPR_AREA_OFF +  5 * FREGBYTES)(sp); \
	op fs6,  (FPR_AREA_OFF +  6 * FREGBYTES)(sp); \
	op fs7,  (FPR_AREA_OFF +  7 * FREGBYTES)(sp); \
	op fs8,  (FPR_AREA_OFF +  8 * FREGBYTES)(sp); \
	op fs9,  (FPR_AREA_OFF +  9 * FREGBYTES)(sp); \
	op fs10, (FPR_AREA_OFF + 10 * FREGBYTES)(sp); \
	op fs11, (FPR_AREA_OFF + 11 * FREGBYTES)(sp)

#define PM_S2RAM_SAVE_FP \
	csrr t0, mstatus; \
	li t1, MSTATUS_FS; \
	and t0, t0, t1; \
	beqz t0, 99f; \
	DO_STACK_FPRS(fsr); \
	frcsr t0; \
	sr t0, FCSR_OFF(sp); \
	99:

#define PM_S2RAM_LOAD_FP \
	csrr t0, mstatus; \
	li t1, MSTATUS_FS; \
	and t0, t0, t1; \
	beqz t0, 98f; \
	DO_STACK_FPRS(flr); \
	lr t0, FCSR_OFF(sp); \
	fscsr t0; \
	98:
#else
#define PM_S2RAM_SAVE_FP
#define PM_S2RAM_LOAD_FP
#endif


GTEXT(arch_pm_s2ram_suspend)
GTEXT(arch_pm_s2ram_resume)
GTEXT(pm_s2ram_mark_set)
GTEXT(pm_s2ram_mark_check_and_clear)
GDATA(_cpu_context)


SECTION_FUNC(TEXT, arch_pm_s2ram_suspend)
	/* a0 = system_off function pointer. */
	addi sp, sp, -STACK_FRAME_BYTES
	DO_STACK_GPRS(sr)
	PM_S2RAM_SAVE_FP

	/* Stash system_off in a callee-saved register so it survives the
	 * pm_s2ram_mark_set() call below.
	 */
	mv s0, a0

	.option push
	.option norelax
	la a1, _cpu_context
	.option pop
	sr sp, ___cpu_context_t_sp_OFFSET(a1)
	csrr t0, mstatus
	sr t0, ___cpu_context_t_mstatus_OFFSET(a1)
	csrr t0, mtvec
	sr t0, ___cpu_context_t_mtvec_OFFSET(a1)
	csrr t0, mscratch
	sr t0, ___cpu_context_t_mscratch_OFFSET(a1)
#ifdef CONFIG_RISCV_HAS_CLIC
	csrr t0, CSR_MTVT
	sr t0, ___cpu_context_t_mtvt_OFFSET(a1)
#else
	csrr t0, mie
	sr t0, ___cpu_context_t_mie_OFFSET(a1)
#endif

	call pm_s2ram_mark_set

	/* Order context/marker stores before any SoC cache maintenance. */
	fence rw, rw

	/* On success this never returns; on abort, fall through. system_off
	 * must preserve mstatus across return (abort uses live FS bits).
	 */
	jalr s0

	/* Abort path: a0 holds system_off()'s return value; preserve it
	 * across the mark_check_and_clear() call.
	 */
	mv s0, a0
	call pm_s2ram_mark_check_and_clear
	mv a0, s0

	PM_S2RAM_LOAD_FP
	DO_STACK_GPRS(lr)
	addi sp, sp, STACK_FRAME_BYTES
	ret


SECTION_FUNC(TEXT, arch_pm_s2ram_resume)
	/* Save current ra so we can return to the SoC wake handler if the
	 * marker says "not resuming" (cold boot path). 16 bytes keeps sp
	 * aligned per the psABI.
	 */
	addi sp, sp, -16
	sr ra, 0(sp)

	call pm_s2ram_mark_check_and_clear

	lr ra, 0(sp)
	addi sp, sp, 16

	/* a0 = true if we should resume the kernel; false = cold boot. */
	bnez a0, .L_resume
	ret

.L_resume:
	.option push
	.option norelax
	la a0, _cpu_context
	.option pop

	/* Defer mstatus write until after FP restore: the saved FS bits
	 * may demand a powered FPU, and we should not arm FS until we are
	 * about to touch FP regs.
	 */
	lr t1, ___cpu_context_t_mstatus_OFFSET(a0)

	/* mscratch before mtvec: _isr_wrapper reads mscratch on entry. */
	lr t0, ___cpu_context_t_mscratch_OFFSET(a0)
	csrw mscratch, t0
	lr t0, ___cpu_context_t_mtvec_OFFSET(a0)
	csrw mtvec, t0
#ifdef CONFIG_RISCV_HAS_CLIC
	lr t0, ___cpu_context_t_mtvt_OFFSET(a0)
	csrw CSR_MTVT, t0
#else
	lr t0, ___cpu_context_t_mie_OFFSET(a0)
	csrw mie, t0
#endif

	lr sp, ___cpu_context_t_sp_OFFSET(a0)

#ifdef CONFIG_FPU
	/* Skip FP restore (and FPU access) if suspending FS == Off. */
	li t2, MSTATUS_FS
	and t0, t1, t2
	beqz t0, 97f
	csrs mstatus, t2
	DO_STACK_FPRS(flr)
	lr t0, FCSR_OFF(sp)
	fscsr t0
97:
#endif

	DO_STACK_GPRS(lr)
	addi sp, sp, STACK_FRAME_BYTES

	csrw mstatus, t1

	li a0, 0
	ret
