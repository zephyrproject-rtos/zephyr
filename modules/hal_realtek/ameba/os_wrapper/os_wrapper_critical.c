/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_wrapper.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
LOG_MODULE_REGISTER(os_if_critical);

/* SMP: per-component spinlock + per-CPU nesting.  UP: single irq_lock. */
#ifdef CONFIG_SMP

static struct k_spinlock critical_locks[RTOS_CRITICAL_MAX];
static k_spinlock_key_t critical_keys[CONFIG_MP_MAX_NUM_CPUS][RTOS_CRITICAL_MAX];
static uint32_t ulCriticalNesting[CONFIG_MP_MAX_NUM_CPUS][RTOS_CRITICAL_MAX];

#else /* !CONFIG_SMP */

static int key;
static volatile uint32_t critical_nesting;

#endif /* CONFIG_SMP */

int rtos_critical_is_in_interrupt(void)
{
#if defined(CONFIG_CPU_AARCH32_CORTEX_A)
	return (__get_mode() != CPSR_M_USR) && (__get_mode() != CPSR_M_SYS);
#else
	return __get_IPSR() != 0;
#endif
}

void rtos_critical_enter(uint32_t component_id)
{
#ifdef CONFIG_SMP
	unsigned int flags;
	unsigned int cpu;

	if (component_id >= RTOS_CRITICAL_MAX) {
		component_id = RTOS_CRITICAL_DEFAULT;
	}

	/* IRQs stay off from the cpu_id read through k_spin_lock to prevent migration. */
	flags = arch_irq_lock();
	cpu = arch_curr_cpu()->id;

	if (ulCriticalNesting[cpu][component_id] == 0) {
		/*
		 * k_spin_lock's key records "IRQs already off", so store 'flags' (the
		 * caller's IRQ state) instead. The outermost exit restores it.
		 */
		(void)k_spin_lock(&critical_locks[component_id]);
		critical_keys[cpu][component_id] = (k_spinlock_key_t){.key = (int)flags};
	}
	/* Nested entry: 'flags' is discarded; the outermost key holds the caller's state. */
	ulCriticalNesting[cpu][component_id]++;
#else
	ARG_UNUSED(component_id);
	if (critical_nesting == 0) {
		key = irq_lock();
	}
	critical_nesting++;
#endif
}

void rtos_critical_exit(uint32_t component_id)
{
#ifdef CONFIG_SMP
	unsigned int cpu;

	if (component_id >= RTOS_CRITICAL_MAX) {
		component_id = RTOS_CRITICAL_DEFAULT;
	}

	cpu = arch_curr_cpu()->id; /* stable: IRQs disabled by held spinlock */

	if (ulCriticalNesting[cpu][component_id] == 0) {
		LOG_ERR("%s: unbalanced exit on CPU %u id=%u", __func__, cpu, component_id);
		return;
	}
	ulCriticalNesting[cpu][component_id]--;
	if (ulCriticalNesting[cpu][component_id] == 0) {
		k_spin_unlock(&critical_locks[component_id], critical_keys[cpu][component_id]);
	}
#else
	ARG_UNUSED(component_id);
	if (critical_nesting == 0) {
		LOG_ERR("%s: unbalanced exit", __func__);
		return;
	}
	critical_nesting--;
	if (critical_nesting == 0) {
		irq_unlock(key);
	}
#endif
}

uint32_t rtos_get_critical_state(void)
{
#ifdef CONFIG_SMP
	unsigned int flags = arch_irq_lock(); /* stabilize cpu_id */
	unsigned int cpu = arch_curr_cpu()->id;
	uint32_t depth = 0;

	for (int i = 0; i < RTOS_CRITICAL_MAX; i++) {
		depth += ulCriticalNesting[cpu][i];
	}
	arch_irq_unlock(flags);
	return depth;
#else
	return critical_nesting;
#endif
}
