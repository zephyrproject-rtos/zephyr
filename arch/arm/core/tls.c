/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <app_memory/app_memdomain.h>
#include <sys/util.h>

#ifdef CONFIG_CPU_CORTEX_M
/*
 * Since Cortex-M does not have the thread ID or process ID
 * register needed to store TLS pointer at runtime for
 * toolchain to access thread data. Use a global variable
 * instead.
 */
K_APP_DMEM(z_libc_partition) uintptr_t z_arm_tls_ptr;
#endif

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for ARM has some data fields following by
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
