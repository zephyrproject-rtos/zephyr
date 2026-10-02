/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception handlers for ARM Cortex-A and Cortex-R
 *
 * This file implements the exception handlers (undefined instruction, prefetch
 * abort and data abort) for ARM Cortex-A and Cortex-R processors.
 *
 * All exception handlers save the exception stack frame into the exception
 * mode stack rather than the system mode stack, in order to ensure predictable
 * exception behaviour (i.e. an arbitrary thread stack overflow cannot cause
 * exception handling and thereby subsequent total system failure).
 *
 * In case the exception is due to a fatal (unrecoverable) fault, the fault
 * handler is responsible for invoking the architecture fatal exception handler
 * (z_arm_fatal_error) which invokes the kernel fatal exception handler
 * (z_fatal_error) that either locks up the system or aborts the current thread
 * depending on the application exception handler implementation.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

#if defined(CONFIG_FPU_SHARING)
GTEXT(z_arm_fault_undef_instruction_fp)
#endif
GTEXT(z_arm_fault_undef_instruction)
GTEXT(z_arm_fault_prefetch)
GTEXT(z_arm_fault_data)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
/* _callee_saved is 36 bytes. Round up so that SP stays 8-byte aligned. */
#define EXC_CALLEE_SAVED_SPACE ((___callee_saved_t_SIZEOF + 7) & ~7)
#endif

#ifndef CONFIG_USE_SWITCH

/*
 * After r0-r3, r12, lr^ are stored at [sp], capture previous-mode SP (in r1)
 * and leave LR as the user/system value stmfd ^ stored. Clobbers r0-r2.
 * Cannot CPS into USR. SP is the value before this frame was pushed.
 *
 * ldmia ^ reloads [sp, #20] into r14_usr. SVC, IRQ, ABT, UND, and FIQ have
 * a banked LR; copying it here would corrupt r14_usr on a recoverable
 * return. System mode shares r14 with User mode, so that slot is already
 * the interrupted LR.
 *
 * Do not msr SPSR into CPSR: the T bit would switch this ARM handler to Thumb.
 * Keep current CPSR (ARM, I/F) and replace only the mode field.
 */
.macro exception_capture_prev_sp
	mrs r0, cpsr
	mrs r1, spsr
	and r2, r1, #MODE_MASK
	cmp r2, #MODE_USR
	beq 1f
	bic r1, r0, #MODE_MASK
	orr r1, r1, r2
	orr r1, r1, #(I_BIT | F_BIT)
	msr CPSR_c, r1
	mov r1, sp
	msr CPSR_c, r0
	/*
	 * A nested abort or undef taken in the same mode reads back the SP
	 * that just built this frame. Add back what was pushed so far:
	 * __basic_sf, the SP word and the pad.
	 */
	and r0, r0, #MODE_MASK
	cmp r0, r2
	addeq r1, r1, #(32 + Z_ARM_ESF_TRAILER_SIZE)
	b 2f
1:
	sub r0, sp, #4
	stmia r0, {sp}^
	ldr r1, [r0]
2:
.endm

.macro exception_entry mode
	/*
	 * ESF on the exception-mode stack (not the thread stack):
	 *   [fpu] [r0-r3, r12, lr, pc, xpsr] [sp_prev] [pad]
	 * sp_prev sits after __basic_sf so SVC/IRQ frames that omit it still
	 * match struct arch_esf.basic. The pad keeps SP 8-byte aligned at
	 * every call into C.
	 */
	sub sp, #Z_ARM_ESF_TRAILER_SIZE
	srsdb sp!, #\mode
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24
	exception_capture_prev_sp
	str r1, [sp, #32]

#if defined(CONFIG_FPU_SHARING)
	sub sp, #___fpu_t_SIZEOF

	vmrs r1, fpexc
	mov r0, #FPEXC_EN
	vmsr fpexc, r0
	vmrs r0, fpscr

	mov r2, sp
	vstmia r2!, {s0-s15}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	vstmia r2!, {d16-d31}
#endif
	stm r2, {r0, r1}
#endif

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Pointer to extra esf info */
	sub sp, #___extra_esf_info_t_SIZEOF
	mov r0, #0
	str r0, [sp, #4]

	sub r1, sp, #EXC_CALLEE_SAVED_SPACE
	str r1, [sp]
	cps #MODE_SYS
	stm r1, {r4-r11, sp}
	cps #\mode

	mov r0, sp
	mov sp, r1
#else
	mov r0, sp
#endif

	/* Increment exception nesting count */
	get_cpu r2
	ldr r1, [r2, #___cpu_t_nested_OFFSET]
	add r1, r1, #1
	str r1, [r2, #___cpu_t_nested_OFFSET]
.endm

.macro exception_exit
	/* Exit exception */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	add sp, #___extra_esf_info_t_SIZEOF
	add sp, #EXC_CALLEE_SAVED_SPACE
#endif
.endm

/**
 * @brief Undefined instruction exception handler
 *
 * An undefined instruction (UNDEF) exception is generated when an undefined
 * instruction, or a VFP instruction when the VFP is not enabled, is
 * encountered.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_undef_instruction)
	/*
	 * The undefined instruction address is offset by 2 if the previous
	 * mode is Thumb; otherwise, it is offset by 4.
	 */
	push {r0}
	mrs r0, spsr
	tst r0, #T_BIT
	subeq lr, #4	/* ARM   (!T_BIT) */
	subne lr, #2	/* Thumb (T_BIT) */
	pop {r0}

	/*
	 * Store r0-r3, r12, lr, lr_und and spsr_und into the stack to
	 * construct an exception stack frame.
	 */
	sub sp, #Z_ARM_ESF_TRAILER_SIZE
	srsdb sp!, #MODE_UND
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24
	exception_capture_prev_sp
	str r1, [sp, #32]

	/* Increment exception nesting count */
	get_cpu r2
	ldr r1, [r2, #___cpu_t_nested_OFFSET]
	add r1, r1, #1
	str r1, [r2, #___cpu_t_nested_OFFSET]

#if defined(CONFIG_FPU_SHARING)
	sub sp, #___fpu_t_SIZEOF

	bl z_arm_fault_undef_instruction_fp
	cmp r0, #0
	beq z_arm_exc_exit

	vmrs r1, fpexc
	mov r0, #FPEXC_EN
	vmsr fpexc, r0
	vmrs r0, fpscr

	mov r2, sp
	vstmia r2!, {s0-s15}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
	vstmia r2!, {d16-d31}
#endif
	stm r2, {r0, r1}
#endif

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Pointer to extra esf info */
	sub sp, #___extra_esf_info_t_SIZEOF
	mov r0, #0
	str r0, [sp, #4]

	sub r1, sp, #EXC_CALLEE_SAVED_SPACE
	str r1, [sp]
	cps #MODE_SYS
	stm r1, {r4-r11, sp}
	cps #MODE_UND

	mov r0, sp
	mov sp, r1
#else
	mov r0, sp
#endif

	bl z_arm_fault_undef_instruction
	exception_exit

	b z_arm_exc_exit

/**
 * @brief Prefetch abort exception handler
 *
 * A prefetch abort (PABT) exception is generated when the processor marks the
 * prefetched instruction as invalid and the instruction is executed.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_prefetch_abort)
	/*
	 * The faulting instruction address is always offset by 4 for the
	 * prefetch abort exceptions.
	 */
	sub lr, #4

	exception_entry MODE_ABT
	bl z_arm_fault_prefetch
	exception_exit

	b z_arm_exc_exit

/**
 * @brief Data abort exception handler
 *
 * A data abort (DABT) exception is generated when an error occurs on a data
 * memory access. This exception can be either synchronous or asynchronous,
 * depending on the type of fault that caused it.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_data_abort)
	/*
	 * The faulting instruction address is always offset by 8 for the data
	 * abort exceptions.
	 */
	sub lr, #8

	exception_entry MODE_ABT
	bl z_arm_fault_data

	exception_exit

	b z_arm_exc_exit

#else

GTEXT(z_arm_cortex_ar_exit_exc)
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_cortex_ar_exit_exc)

	/* Note:
	 * This function is expected to be *always* called with
	 * processor mode set to MODE_SYS.
	 */

	/* decrement exception depth */
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	sub r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

	/*
	 * Restore r0-r3, r12, lr, lr_und and spsr_und from the exception stack
	 * and return to the current thread.
	 */
	pop {r0-r3, r12, lr}
	rfeia sp!

/**
 * @brief Undefined instruction exception handler
 *
 * An undefined instruction (UNDEF) exception is generated when an undefined
 * instruction, or a VFP instruction when the VFP is not enabled, is
 * encountered.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_undef_instruction)
	/*
	 * The undefined instruction address is offset by 2 if the previous
	 * mode is Thumb; otherwise, it is offset by 4.
	 */
	push {r0}
	mrs r0, spsr
	tst r0, #T_BIT
	subeq lr, #4	/* ARM   (!T_BIT) */
	subne lr, #2	/* Thumb (T_BIT) */
	pop {r0}

	z_arm_cortex_ar_enter_exc
	bl z_arm_fault_undef_instruction
	b z_arm_cortex_ar_exit_exc

/**
 * @brief Prefetch abort exception handler
 *
 * A prefetch abort (PABT) exception is generated when the processor marks the
 * prefetched instruction as invalid and the instruction is executed.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_prefetch_abort)
	/*
	 * The faulting instruction address is always offset by 4 for the
	 * prefetch abort exceptions.
	 */
	sub lr, #4
	z_arm_cortex_ar_enter_exc
	bl z_arm_fault_prefetch
	b z_arm_cortex_ar_exit_exc

/**
 * @brief Data abort exception handler
 *
 * A data abort (DABT) exception is generated when an error occurs on a data
 * memory access. This exception can be either synchronous or asynchronous,
 * depending on the type of fault that caused it.
 */
SECTION_SUBSEC_FUNC(TEXT, __exc, z_arm_data_abort)
	sub lr, #8

	z_arm_cortex_ar_enter_exc
	bl z_arm_fault_data
	b z_arm_cortex_ar_exit_exc

#endif
