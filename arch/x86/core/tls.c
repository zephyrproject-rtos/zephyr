/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/sys/util.h>

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for x86 and x86_64 has the data/bss first,
	 * then a pointer pointing to itself. The address to
	 * this pointer needs to be stored in register GS (x86)
	 * or FS (x86_64). GCC generates code which reads this
	 * pointer and offsets from this pointer are used to
	 * access data.
	 */

	uintptr_t *self_ptr;

	/*
	 * Since we are populating things backwards, store
	 * the pointer to the TLS area at top of stack.
	 */
	stack_ptr -= sizeof(uintptr_t);
	self_ptr = (void *)stack_ptr;
	*self_ptr = POINTER_TO_UINT(stack_ptr);

	/*
	 * Set thread TLS pointer as this is used to populate
	 * FS/GS at context switch.
	 */
	new_thread->tls = POINTER_TO_UINT(self_ptr);

	/* Setup the TLS data */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	return (z_tls_data_size() + sizeof(uintptr_t));
}
