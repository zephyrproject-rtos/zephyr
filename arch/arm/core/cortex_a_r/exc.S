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
GTEXT(z_arm_fault_stack_overflow)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

#ifndef CONFIG_USE_SWITCH

.macro exception_entry mode
	/*
	 * Store r0-r3, r12, lr, lr_und and spsr_und into the stack to
	 * construct an exception stack frame.
	 */
	srsdb sp!, #\mode
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24

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
	str r0, [sp, #8]

	sub r1, sp, #___callee_saved_t_SIZEOF
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
	add sp, #___callee_saved_t_SIZEOF
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
	srsdb sp!, #MODE_UND
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24

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
	str r0, [sp, #8]

	sub r1, sp, #___callee_saved_t_SIZEOF
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

#if defined(CONFIG_FPU_SHARING)
	/*
	 * Thread code runs with the FPU enabled only if it owns the FPU.
	 * Exception handlers run with it disabled, see z_arm_fpu_trap().
	 */
	mov r3, #0
	cmp r1, #0
	bne 1f
	ldr r0, [r2, #_cpu_offset_to_fpu_owner]
	ldr r1, [r2, #___cpu_t_current_OFFSET]
	cmp r0, r1
	moveq r3, #FPEXC_EN
1:
	vmsr fpexc, r3
#endif

	/*
	 * Restore r0-r3, r12, lr, lr_und and spsr_und from the exception stack
	 * and return to the current thread.
	 */
	ldr sp, [sp]
	pop {r0-r3, r12, lr}
	userspace_exc_exit
	rfeia sp!

#if defined(CONFIG_FPU_SHARING)
/*
 * Exit from z_arm_fpu_trap(), which leaves the FPU enabled for the
 * faulting instruction to be retried.
 */
z_arm_fpu_trap_exit:
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	sub r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

	ldr sp, [sp]
	pop {r0-r3, r12, lr}
	userspace_exc_exit
	rfeia sp!
#endif

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

	z_arm_cortex_ar_enter_exc 0
#if defined(CONFIG_FPU_SHARING)
	/* A VFP instruction with the FPU disabled: lazy FPU switch */
	bl z_arm_fpu_trap
	cmp r0, #0
	bne z_arm_fpu_trap_exit
#endif
	z_arm_cortex_ar_save_extra
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
#if defined(CONFIG_MPU_STACK_GUARD)
	/* Store LR and SPSR for the stack overflow path, see z_arm_data_abort */
	sub sp, #4
	srsdb sp, #MODE_ABT
	add sp, #4
#endif
	z_arm_cortex_ar_enter_exc
	z_arm_cortex_ar_save_extra
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

#if defined(CONFIG_MPU_STACK_GUARD)
	/*
	 * Exception handlers run in SYS mode. A data abort taken in another
	 * mode comes from an exception entry whose frame push hit the MPU
	 * guard of the stack, unless it is an asynchronous abort.
	 */
	push {r0}
	mrs r0, spsr
	and r0, r0, #MODE_MASK
	cmp r0, #MODE_SYS
	cmpne r0, #MODE_USR
	bne stack_overflow
	/*
	 * The frame push below may hit the guard, and the data abort it
	 * takes overwrites LR and SPSR. Store them below r0 on the abort mode
	 * stack, where the stack overflow path finds them.
	 */
	srsdb sp, #MODE_ABT
	pop {r0}
#endif

	z_arm_cortex_ar_enter_exc
	z_arm_cortex_ar_save_extra
	bl z_arm_fault_data
	b z_arm_cortex_ar_exit_exc

#if defined(CONFIG_MPU_STACK_GUARD)
stack_overflow:
	/*
	 * r0 holds the mode of the exception entry that hit the guard, and
	 * its value is at the top of the abort mode stack. The PC and CPSR
	 * of the context that entry interrupted go right below: a data or
	 * prefetch abort entry stored them there already, and other modes
	 * hold them in their LR and SPSR, which SRS stores from that mode.
	 */
	cmp r0, #MODE_ABT
	beq 1f
	orr r0, r0, #(I_BIT | F_BIT)
	msr cpsr_c, r0
	srsdb sp, #MODE_ABT
	cps #MODE_ABT
1:
	/*
	 * The SYS mode stack is unusable: handle the fatal error on the abort
	 * mode stack, which ISRs don't use, with the SYS mode SP stored right
	 * above the frame. z_arm_fault_stack_overflow() does not return.
	 */
	sub r0, sp, #12
	cps #MODE_SYS
	str sp, [r0]
	mov sp, r0
	cps #MODE_ABT
	pop {r0}
	z_arm_cortex_ar_enter_exc
	z_arm_cortex_ar_save_extra
	bl z_arm_fault_stack_overflow
#endif

#endif
