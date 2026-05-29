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

#ifdef __CCAC__
extern char _arcmwdt_tls_start[];
extern char _arcmwdt_tls_size[];

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	size_t tls_size = (size_t)_arcmwdt_tls_size;
	size_t tls_size_aligned = ROUND_UP(tls_size, ARCH_STACK_PTR_ALIGN);

	stack_ptr -= tls_size_aligned;
	memcpy(stack_ptr, _arcmwdt_tls_start, tls_size);

	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return tls_size_aligned;
}

void *_Preserve_flags _mwget_tls(void)
{
	return (void *)(_current->tls);
}

#else
size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for ARC has some data fields following by
	 * thread data and bss. These fields are supposed to be
	 * used by toolchain and OS TLS code to aid in locating
	 * the TLS data/bss. Zephyr currently has no use for
	 * this so we can simply skip these. However, since GCC
	 * is generating code assuming these fields are there,
	 * we simply skip them when setting the TLS pointer.
	 */

	/*
	 * Since we are populating things backwards,
	 * setup the TLS data/bss area first.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/* Skip two pointers due to toolchain */
	stack_ptr -= sizeof(uintptr_t) * 2;

	/*
	 * Set thread TLS pointer which is used in
	 * context switch to point to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return (z_tls_data_size() + (sizeof(uintptr_t) * 2));
}
#endif
