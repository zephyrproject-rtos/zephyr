/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <arch/x86/acpi.h>
#include <arch/x86/multiboot.h>

extern FUNC_NORETURN void z_cstart(void);

FUNC_NORETURN void z_x86_prep_c(int unused, struct multiboot_info *info)
{
	ARG_UNUSED(unused);

	_kernel.cpus[0].nested = 0;
	_kernel.cpus[0].irq_stack = Z_THREAD_STACK_BUFFER(_interrupt_stack) +
					CONFIG_ISR_STACK_SIZE;

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
	z_x86_early_serial_init();
#endif

#ifdef CONFIG_MULTIBOOT_INFO
	z_multiboot_init(info);
#else
	ARG_UNUSED(info);
#endif

#ifdef CONFIG_ACPI
	z_acpi_init();
#endif

#ifdef CONFIG_X86_MMU
	z_x86_paging_init();
#endif

#if CONFIG_X86_STACK_PROTECTION
	z_x86_mmu_set_flags(&z_x86_kernel_ptables, _interrupt_stack,
			    MMU_PAGE_SIZE, MMU_ENTRY_READ, Z_X86_MMU_RW,
			    true);
#endif

#if defined(CONFIG_SMP)
	z_x86_ipi_setup();
#endif

	z_cstart();
}
