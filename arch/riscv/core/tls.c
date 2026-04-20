/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>

/* Non-inline wrapper for z_tls_data_size(), required for calling from assembly. */
size_t FUNC_NO_STACK_PROTECTOR z_tls_data_size_asm(void)
{
	return z_tls_data_size();
}

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	/*
	 * TLS area for RISC-V is simple without any extra data.
	 */

	/*
	 * Since we are populating things backwards, setup the TLS data/bss
	 * area first.
	 */
	stack_ptr -= z_tls_data_size();
	z_tls_copy(stack_ptr);

	/*
	 * Set thread TLS pointer which is used in context switch to point
	 * to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);

	return z_tls_data_size();
}

void FUNC_NO_STACK_PROTECTOR arch_riscv_early_tls_stack_update(char *stack_ptr, size_t tls_size)
{
	/*
	 * TLS area for RISC-V is simple without any extra data.
	 */

	/*
	 * Since we are populating things backwards, setup the TLS data/bss
	 * area first. tls_size was already computed by the caller
	 * (z_tls_data_size_asm) to avoid a redundant call.
	 */
	stack_ptr -= tls_size;
	z_tls_copy(stack_ptr);

	/*
	 * Set tp directly to point to the TLS area, since there are no
	 * threads spawned early in boot.
	 */
	uintptr_t tls_addr = POINTER_TO_UINT(stack_ptr);

	__asm__ volatile ("mv tp, %0" :: "r"(tls_addr));
}
