/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel/internal/critical_section_hooks.h>
#include <zephyr/llext/symbol.h>

#define CRITICAL_SECTION_FN __noinline __no_instrumentation__ __noasan

#ifndef CONFIG_SMP
CRITICAL_SECTION_FN unsigned int z_crit_irq_lock(void)
{
	unsigned int key = arch_irq_lock();

	z_crit_hook_irq_begin(key, Z_CRITICAL_SECTION_CALLER());
	return key;
}
EXPORT_SYMBOL(z_crit_irq_lock);

CRITICAL_SECTION_FN void z_crit_irq_unlock(unsigned int key)
{
	z_crit_hook_irq_end(key);
	arch_irq_unlock(key);
}
EXPORT_SYMBOL(z_crit_irq_unlock);
#endif /* !CONFIG_SMP */

CRITICAL_SECTION_FN void z_crit_irq_idle_enter(void)
{
	unsigned int key = arch_irq_lock();

	z_crit_hook_irq_idle(key);
	arch_irq_unlock(key);
}
EXPORT_SYMBOL(z_crit_irq_idle_enter);

/* Inline kernel APIs also use these hooks from loadable extensions. */
EXPORT_SYMBOL(z_crit_hook_irq_end);
EXPORT_SYMBOL(z_crit_hook_spin_acquired);
EXPORT_SYMBOL(z_crit_hook_spin_unlocked);
EXPORT_SYMBOL(z_crit_hook_spin_released);
#ifdef CONFIG_SMP
EXPORT_SYMBOL(z_crit_hook_spin_attempt);
EXPORT_SYMBOL(z_crit_hook_spin_abort);
#endif /* CONFIG_SMP */
