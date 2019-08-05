/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>

FUNC_NORETURN void z_x86_prep_c(void)
{
	_kernel.nested = 0;
	_kernel.irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack) +
				CONFIG_ISR_STACK_SIZE;

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
	z_x86_early_serial_init();
#endif
#ifdef CONFIG_X86_MMU
	z_x86_paging_init();
#endif
#if CONFIG_X86_STACK_PROTECTION
	z_x86_mmu_set_flags(&z_x86_kernel_pdpt, _interrupt_stack, MMU_PAGE_SIZE,
			    MMU_ENTRY_READ, MMU_PTE_RW_MASK, true);
#endif

	z_cstart();
}
