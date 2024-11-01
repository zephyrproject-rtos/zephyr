/*
 * Copyright (c) 2024 Meta Platforms.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ARCH_COMMON_ARCH_INLINES_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ARCH_COMMON_ARCH_INLINES_H_

#ifndef ZEPHYR_INCLUDE_ARCH_INLINES_H_
#error "This header shouldn't be included directly"
#endif /* ZEPHYR_INCLUDE_ARCH_INLINES_H_ */

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL
static ALWAYS_INLINE struct k_thread *arch_current_thread(void)
{
#ifdef CONFIG_SMP
	/* In SMP, _current is a field read from _current_cpu, which
	 * can race with preemption before it is read.  We must lock
	 * local interrupts when reading it.
	 */
	unsigned int k = arch_irq_lock();

	struct k_thread *ret = _current_cpu->current;

	arch_irq_unlock(k);
#else
	struct k_thread *ret = _kernel.cpus[0].current;
#endif /* CONFIG_SMP */
	return ret;
}

static ALWAYS_INLINE void arch_current_thread_set(struct k_thread *thread)
{
	_current_cpu->current = thread;
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ARCH_COMMON_ARCH_INLINES_H_ */
