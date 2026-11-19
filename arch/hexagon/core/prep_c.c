/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

#ifdef CONFIG_THREAD_LOCAL_STORAGE
#include <kernel_tls.h>
#endif

extern char _interrupt_stack[];

/**
 * z_prep_c - Architecture-specific C entry point.
 *
 * Called from the reset handler in hvm_event_vectors.S after BSS has
 * been cleared and the MMU page table configured.  Performs any
 * remaining arch-level setup and hands off to the kernel via z_cstart().
 */
FUNC_NORETURN void z_prep_c(void)
{
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/*
	 * Boot code reads thread-locals (arch_is_user_context()) before any
	 * thread's UGP is switched in. Give it a block at the bottom of the
	 * boot stack, laid out as arch_tls_stack_setup() does.
	 */
	char *block = (char *)ROUND_UP((uintptr_t)_interrupt_stack, 8);

	z_tls_copy(block);
	__asm__ volatile("ugp = %0" : : "r"(block + ROUND_UP(z_tls_data_size(), 8)));
#endif

	z_cstart();
	CODE_UNREACHABLE;
}
