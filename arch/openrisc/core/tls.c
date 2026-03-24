/*
 * Copyright (c) 2026 Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>

#define TLS_OFFSET	0

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for OpenRISC has 16 bytes for a
	 * thread control block followed by the TLS
	 * data.
	 */

	/*
	 * Since we are populating things backwards,
	 * setup the TLS data/bss area first.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/* Skip due to toolchain */
	stack_ptr -= TLS_OFFSET;

	/*
	 * Set thread TLS pointer which is used in
	 * context switch to point to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return z_tls_data_size() + TLS_OFFSET;
}
