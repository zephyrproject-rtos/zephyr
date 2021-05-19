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

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_fault_undef_instruction)
GTEXT(z_arm_fault_prefetch)
GTEXT(z_arm_fault_data)

GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

.macro exception_entry mode
	/*
	 * Store r0-r3, r12, lr, lr_und and spsr_und into the stack to
	 * construct an exception stack frame.
	 */
	srsdb sp!, #\mode
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24

	/*
	 * Create new esf struct for exception handler debug.  The first
	 * time the basic stack frame is saved is for getting in and out
	 * of the exception.
	 */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	sub sp, #___callee_saved_t_SIZEOF
	sub sp, #___extra_esf_info_t_SIZEOF
#endif

	srsdb sp!, #\mode
	stmfd sp, {r0-r3, r12, lr}^
	sub sp, #24

	/* Increment exception nesting count */
	ldr r2, =_kernel
	ldr r0, [r2, #_kernel_offset_to_nested]
	add r0, r0, #1
	str r0, [r2, #_kernel_offset_to_nested]

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	/* Pointer to extra esf info */
	add r0, sp, #___basic_sf_t_SIZEOF
	mov r1, #0
	str r1, [r0, #4]
	str r1, [r0, #8]

	/* Pointer to callee saved registers */
	add r1, r0, #___extra_esf_info_t_SIZEOF
	str r1, [r0]

	cps #MODE_SYS
	stm r1, {r4-r11, sp}
	cps #\mode
#endif

	/* Invoke fault handler */
	mov r0, sp
.endm

.macro exception_exit
	/* Exit exception */
	add sp, sp, #___basic_sf_t_SIZEOF
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

	exception_entry MODE_UND
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
