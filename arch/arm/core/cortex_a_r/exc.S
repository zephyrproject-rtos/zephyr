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
 * The exception handlers run in system mode. They save the exception stack
 * frame on the stack of the interrupted context, or on the privileged stack
 * of a user thread.
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

GTEXT(z_arm_fault_undef_instruction)
GTEXT(z_arm_fault_prefetch)
GTEXT(z_arm_fault_data)
GTEXT(z_arm_fault_stack_overflow)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

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

exc_return:
	/*
	 * Restore r0-r3, r12, lr, PC and CPSR from the exception stack frame
	 * and return to the current thread.
	 */
	z_arm_cortex_ar_exc_return

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
	b exc_return
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
	pop {r0}
	bne stack_overflow
#endif

	z_arm_cortex_ar_enter_exc
	z_arm_cortex_ar_save_extra
	bl z_arm_fault_data
	b z_arm_cortex_ar_exit_exc

#if defined(CONFIG_MPU_STACK_GUARD)
stack_overflow:
	/*
	 * The SYS mode stack is unusable: handle the fatal error on the abort
	 * mode stack, which ISRs don't use, from 8 bytes below its top. The
	 * entry of a nested data abort uses the top 4 bytes.
	 * z_arm_fault_stack_overflow() does not return.
	 */
	push {r0}
	sub r0, sp, #4
	cps #MODE_SYS
	mov sp, r0
	cps #MODE_ABT
	pop {r0}
	z_arm_cortex_ar_enter_exc
	z_arm_cortex_ar_save_extra
	bl z_arm_fault_stack_overflow
#endif
