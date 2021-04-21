/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
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
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	return z_tls_data_size();
}
