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

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for RISC-V is simple without any extra
	 * data.
	 */

	/*
	 * Since we are populating things backwards,
	 * setup the TLS data/bss area first.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/*
	 * Set thread TLS pointer which is used in
	 * context switch to point to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return z_tls_data_size();
}
