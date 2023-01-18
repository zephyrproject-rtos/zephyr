/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <microblaze/microblaze_regs.h>
#include <microblaze/microblaze_asm.h>


/* import */
.extern stack_pointer_on_exception_entry
.extern _Fault

/* export */
.global _exception_handler_entry
.global _asm_stack_failed


	.text
	.balign 4

_exception_handler_entry:

	/* Make room for the context on the stack. */
	STACK_ALLOC(__z_arch_esf_t_SIZEOF)
	ASSERT_GT_ZERO(r1, _asm_stack_failed)

	/* Take a copy of the stack pointer at the moment w/ context stored,
	storing its value prior to the function stack frame being created. */
	STORE_REG_TO_ADDR(r1, stack_pointer_on_exception_entry)

	/* Stack MSR using r11(temp1) */
	mfs TEMP_DATA_REG, rmsr
	STORE_TO_STACK(TEMP_DATA_REG, ESF_OFFSET(msr))

	#if defined(CONFIG_USE_HARDWARE_FLOAT_INSTR)
		/* Stack FSR using TEMP_DATA_REG(temp1) */
		mfs TEMP_DATA_REG, rfsr
		STORE_TO_STACK(TEMP_DATA_REG, ESF_OFFSET(fsr))
	#endif

	PUSH_CONTEXT_TO_STACK(r31)
	PUSH_CONTEXT_TO_STACK(r30)
	PUSH_CONTEXT_TO_STACK(r29)
	PUSH_CONTEXT_TO_STACK(r28)
	PUSH_CONTEXT_TO_STACK(r27)
	PUSH_CONTEXT_TO_STACK(r26)
	PUSH_CONTEXT_TO_STACK(r25)
	PUSH_CONTEXT_TO_STACK(r24)
	PUSH_CONTEXT_TO_STACK(r23)
	PUSH_CONTEXT_TO_STACK(r22)
	PUSH_CONTEXT_TO_STACK(r21)
	PUSH_CONTEXT_TO_STACK(r20)
	PUSH_CONTEXT_TO_STACK(r19)
	PUSH_CONTEXT_TO_STACK(r18)
	PUSH_CONTEXT_TO_STACK(r17)
	PUSH_CONTEXT_TO_STACK(r16)
	PUSH_CONTEXT_TO_STACK(r15)
	PUSH_CONTEXT_TO_STACK(r14)
	PUSH_CONTEXT_TO_STACK(r13)
	PUSH_CONTEXT_TO_STACK(r12)
	PUSH_CONTEXT_TO_STACK(r11)
	PUSH_CONTEXT_TO_STACK(r10)
	PUSH_CONTEXT_TO_STACK(r9)
	PUSH_CONTEXT_TO_STACK(r8)
	PUSH_CONTEXT_TO_STACK(r7)
	PUSH_CONTEXT_TO_STACK(r6)
	PUSH_CONTEXT_TO_STACK(r5)
	PUSH_CONTEXT_TO_STACK(r4)
	PUSH_CONTEXT_TO_STACK(r3)
	PUSH_CONTEXT_TO_STACK(r2)

	mfs r5, resr
	mfs r6, rear
	mfs r7, redr

	braid _Fault
	nop

	.text
	.balign 4

_asm_stack_failed:
/* Should always be called with interrupts disabled
 * so that ISR doesn't overwrite irq stack
 * Currently only jumped from isr and swap entry.
 */

	/* stack has failed so we immediately need to switch to an emergency stack */
	SET_REG(r1, z_interrupt_stacks)
	braid _exception_handler_entry
	STACK_FREE(__z_interrupt_stack_SIZEOF)
