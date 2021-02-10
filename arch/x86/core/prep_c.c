/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <arch/x86/acpi.h>
#include <arch/x86/multiboot.h>
#include <x86_mmu.h>

extern FUNC_NORETURN void z_cstart(void);
extern void x86_64_irq_init(void);

/* Early global initialization functions, C domain. This runs only on the first
 * CPU for SMP systems.
 */
FUNC_NORETURN void z_x86_prep_c(void *arg)
{
	struct multiboot_info *info = arg;

	_kernel.cpus[0].nested = 0;

#if defined(CONFIG_LOAPIC)
	z_loapic_enable(0);
#endif

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
	z_x86_early_serial_init();
#endif

#ifdef CONFIG_X86_64
	x86_64_irq_init();
#endif

#ifdef CONFIG_MULTIBOOT_INFO
	z_multiboot_init(info);
#else
	ARG_UNUSED(info);
#endif

#if CONFIG_X86_STACK_PROTECTION
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		z_x86_set_stack_guard(z_interrupt_stacks[i]);
	}
#endif

#if defined(CONFIG_SMP)
	z_x86_ipi_setup();
#endif

	z_cstart();
}
