/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>

FUNC_NO_STACK_PROTECTOR
void z_x86_early_tls_update_gdt(char *stack_ptr)
{
	uintptr_t *self_ptr;
	uint32_t fs_base = X86_FS_BASE;

	/*
	 * Since we are populating things backwards, store
	 * the pointer to the TLS area at top of stack.
	 */
	stack_ptr -= sizeof(uintptr_t);
	self_ptr = (void *)stack_ptr;
	*self_ptr = POINTER_TO_UINT(stack_ptr);

	__asm__ volatile(
		"movl %0, %%ecx;\n\t"
		"movq %1, %%rax;\n\t"
		"movq %1, %%rdx;\n\t"
		"shrq $32, %%rdx;\n\t"
		"wrmsr;\n\t"
		:
		: "r"(fs_base), "r"(POINTER_TO_UINT(self_ptr)));
}
