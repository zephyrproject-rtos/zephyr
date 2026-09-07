/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <kernel_tls.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>

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
	char *const stack_end = stack_ptr;
#ifndef __IAR_SYSTEMS_ICC__
	size_t tls_align = (size_t)(uintptr_t)__tls_align;
#else
	size_t tls_align = 1;
#endif

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
	stack_ptr = (char *)ROUND_DOWN((uintptr_t)stack_ptr - z_tls_data_size(), tls_align);
	z_tls_copy(stack_ptr);

#ifndef __IAR_SYSTEMS_ICC__
	/*
	 * The Arm TLS ABI reserves two pointers before the TLS image and rounds
	 * the image offset up to its alignment. Compiler-generated TLS offsets
	 * include that padding. In particular, an alignas(64) TLS object is at
	 * TP + 64, not immediately after the two-word TCB.
	 */
	size_t toolchain_tls_size = ROUND_UP(sizeof(uintptr_t) * 2U, tls_align);
	stack_ptr -= toolchain_tls_size;
#endif

#if defined(CONFIG_MULTITHREADING)
	/*
	 * Set thread TLS pointer which is used in
	 * context switch to point to TLS area.
	 */
	new_thread->tls = POINTER_TO_UINT(stack_ptr);
#else
	ARG_UNUSED(new_thread);
#endif /* CONFIG_MULTITHREADING */

	return (size_t)(stack_end - stack_ptr);
}
