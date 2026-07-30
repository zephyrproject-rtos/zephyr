/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/fatal.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util_macro.h>
#include <sw_isr_common.h>
#include <vector.h>

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
#include <zephyr/irq_multilevel.h>
#include <zephyr/irq_nextlevel.h>
#endif

/* Reserve non-IRQ CPU vectors so dynamic ISR allocation cannot claim them. */
static void m68k_nonconnectable_vector(const void *arg)
{
	const unsigned int vector = (uintptr_t)arg;
	unsigned int reason = K_ERR_CPU_EXCEPTION;

	printk("m68k: non-connectable CPU vector %u entered IRQ dispatch\n", vector);

	if ((vector == M68K_VECTOR_UNINITIALIZED_INTERRUPT) ||
	    (vector == M68K_VECTOR_SPURIOUS) ||
	    (vector == M68K_VECTOR_NMI)) {
		reason = K_ERR_SPURIOUS_IRQ;
	}

	z_fatal_error(reason, NULL);
}

BUILD_ASSERT((M68K_VECTOR_SPURIOUS + 1) == 25,
	     "update the low reserved-vector declaration count");
BUILD_ASSERT((M68K_VECTOR_RESERVED_HIGH_LAST - M68K_VECTOR_NMI + 1) == 33,
	     "update the high reserved-vector declaration count");

#define M68K_RESERVE_VECTOR(index, base)                                            \
	Z_ISR_DECLARE((base) + (index), 0, m68k_nonconnectable_vector,              \
		      (const void *)(uintptr_t)((base) + (index)));

LISTIFY(25, M68K_RESERVE_VECTOR, (), M68K_VECTOR_RESET_SP)

LISTIFY(33, M68K_RESERVE_VECTOR, (), M68K_VECTOR_NMI)

#undef M68K_RESERVE_VECTOR

/*
 * First-level vectors have no per-source CPU enable state. Cascaded IRQs
 * delegate enable state to their controller.
 */

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
struct m68k_irq_target {
	const struct device *controller;
	unsigned int line;
};

static ALWAYS_INLINE bool m68k_irq_resolve(unsigned int irq,
				    struct m68k_irq_target *target)
{
	const unsigned int level = irq_get_level(irq);

	if (level > 1U) {
		target->controller = z_get_sw_isr_device_from_irq(irq);
		target->line = irq_from_level(irq, level);
		return true;
	} else {
		return false;
	}
}
#endif

void arch_irq_enable(unsigned int irq)
{
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	struct m68k_irq_target target;

	if (m68k_irq_resolve(irq, &target)) {
		irq_enable_next_level(target.controller, target.line);
	}
#else
	ARG_UNUSED(irq);
#endif
}

void arch_irq_disable(unsigned int irq)
{
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	struct m68k_irq_target target;

	if (m68k_irq_resolve(irq, &target)) {
		irq_disable_next_level(target.controller, target.line);
	}
#else
	ARG_UNUSED(irq);
#endif
}

int arch_irq_is_enabled(unsigned int irq)
{
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	struct m68k_irq_target target;

	if (m68k_irq_resolve(irq, &target)) {
		return irq_line_is_enabled_next_level(target.controller, target.line);
	}
#else
	ARG_UNUSED(irq);
#endif

	return true;
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
static bool m68k_first_level_irq_is_connectable(unsigned int vector)
{
	return ((vector >= M68K_VECTOR_AUTOVEC_FIRST) &&
		(vector <= M68K_VECTOR_MASKABLE_AUTOVEC_LAST)) ||
	       ((vector >= M68K_VECTOR_USER_BASE) &&
		(vector <= M68K_VECTOR_USER_LAST));
}

int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	unsigned int level = 1U;
	unsigned int table_idx;
	unsigned int key;
	struct _isr_table_entry *entry;

	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	level = irq_get_level(irq);
#endif

	if (routine == NULL) {
		__ASSERT(false, "cannot install a NULL ISR");
		return -EINVAL;
	}

	if ((level == 1U) && !m68k_first_level_irq_is_connectable(irq)) {
		__ASSERT(false, "CPU vector %u is not connectable as an IRQ", irq);
		return -EINVAL;
	}

	table_idx = z_get_sw_isr_table_idx(irq);
	entry = &_sw_isr_table[table_idx];
	key = arch_irq_lock();

	/*
	 * First-level vectors can only replace z_irq_spurious. Cascaded IRQs may
	 * be replaced while their controller line is disabled.
	 */
	if ((level == 1U) && (entry->isr != z_irq_spurious)) {
		arch_irq_unlock(key);
		__ASSERT(false, "first-level IRQ %u is already connected", irq);
		return -EBUSY;
	}

	if ((level > 1U) && arch_irq_is_enabled(irq)) {
		arch_irq_unlock(key);
		__ASSERT(false, "IRQ %u is enabled", irq);
		return -EBUSY;
	}

#ifdef CONFIG_SHARED_INTERRUPTS
	z_isr_install(irq, routine, parameter);
#else
	entry->arg = parameter;
	entry->isr = routine;
#endif

	arch_irq_unlock(key);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
