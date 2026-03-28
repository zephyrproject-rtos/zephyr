/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <zephyr/arch/x86/ia32/arch.h>
#include <zephyr/arch/x86/ia32/segmentation.h>

#define ENTRY_NUM	(GS_TLS_SEG >> 3)

void z_x86_tls_update_gdt(struct k_thread *thread)
{
	/*
	 * GS is used for thread local storage to pointer to
	 * the TLS storage area in stack. Here we update one
	 * of the descriptor so GS has the new address.
	 *
	 * The re-loading of descriptor into GS is taken care
	 * of inside the assembly swap code just before
	 * swapping into the new thread.
	 */

	struct segment_descriptor *sd = &_gdt.entries[ENTRY_NUM];

	sd->base_low = thread->tls & 0xFFFFU;
	sd->base_mid = (thread->tls >> 16) & 0xFFU;
	sd->base_hi = (thread->tls >> 24) & 0xFFU;
}

FUNC_NO_STACK_PROTECTOR
void z_x86_early_tls_update_gdt(char *stack_ptr)
{
	uintptr_t *self_ptr;
	uintptr_t tls_seg = GS_TLS_SEG;
	struct segment_descriptor *sd = &_gdt.entries[ENTRY_NUM];

	/*
	 * Since we are populating things backwards, store
	 * the pointer to the TLS area at top of stack.
	 */
	stack_ptr -= sizeof(uintptr_t);
	self_ptr = (void *)stack_ptr;
	*self_ptr = POINTER_TO_UINT(stack_ptr);

	sd->base_low = POINTER_TO_UINT(self_ptr) & 0xFFFFU;
	sd->base_mid = (POINTER_TO_UINT(self_ptr) >> 16) & 0xFFU;
	sd->base_hi = (POINTER_TO_UINT(self_ptr) >> 24) & 0xFFU;

	__asm__ volatile(
		"movl %0, %%eax;\n\t"
		"movl %%eax, %%gs;\n\t"
		:
		: "r"(tls_seg));
}
