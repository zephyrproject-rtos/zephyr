/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#ifndef CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER
#include <zephyr/drivers/interrupt_controller/intc_renesas_rh850.h>
#endif

/* Per-CPU saved SP of the outermost interrupted thread.
 * Index by arch_curr_cpu()->id, which is set in z_init_cpu().
 */
static uintptr_t rh850_isr_saved_sp[CONFIG_MP_MAX_NUM_CPUS];

#ifndef CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER

#ifndef CONFIG_RENESAS_RH850_INTC
#error "RH850 requires CONFIG_RENESAS_RH850_INTC or CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER"
#endif

void arch_irq_enable(unsigned int irq)
{
	intc_rh850_irq_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	intc_rh850_irq_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return intc_rh850_irq_is_enabled(irq);
}

void z_rh850_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	intc_rh850_irq_priority_set(irq, prio, flags);
}

#endif

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

bool z_rh850_irq_switch(void **switch_to, void **switch_from)
{
#ifdef CONFIG_USE_SWITCH
	_cpu_t *cpu = arch_curr_cpu();

	/* For nested > 1, we are still inside a nested interrupt and we
	 * must not perform a thread context switch yet.
	 */
	if (cpu->nested != 1U) {
		return false;
	}

	struct k_thread *current = cpu->current;
	void *interrupted_handle = current->switch_handle;

	void *next_handle = z_get_next_switch_handle(interrupted_handle);

	if (next_handle == interrupted_handle) {
		return false;
	}

	if (switch_to != NULL) {
		*switch_to = next_handle;
	}

	if (switch_from != NULL) {
		*switch_from = &current->switch_handle;
	}

	return true;
#else
	ARG_UNUSED(switch_to);
	ARG_UNUSED(switch_from);
	return false;
#endif
}

void z_rh850_isr_enter(void)
{
	_cpu_t *cpu = arch_curr_cpu();

	cpu->nested++;
}

void z_rh850_isr_exit(void)
{
	_cpu_t *cpu = arch_curr_cpu();

	__ASSERT(cpu->nested > 0U, "ISR nested underflow");
	cpu->nested--;
}

uintptr_t z_rh850_isr_stack_enter(uintptr_t current_sp)
{
	_cpu_t *cpu = arch_curr_cpu();

	/* Only the outermost ISR switches to the dedicated IRQ stack. */
	if (cpu->nested == 1U) {
		uint32_t id = cpu->id;

		__ASSERT(id < CONFIG_MP_MAX_NUM_CPUS, "Invalid CPU ID");

		/* Save original thread SP in per-CPU storage. */
		rh850_isr_saved_sp[id] = current_sp;

		/* Switch to per-CPU IRQ stack that Zephyr already initialized
		 * in z_init_cpu(): cpu->irq_stack points to the top of that stack.
		 */
		return (uintptr_t)cpu->irq_stack;
	}

	/* Nested ISR: we are already running on IRQ stack, keep current SP. */
	return current_sp;
}

uintptr_t z_rh850_isr_stack_exit(uintptr_t current_sp)
{
	_cpu_t *cpu = arch_curr_cpu();

	/* cpu->nested still includes the current ISR:
	 *  - nested == 1 → we are exiting the outermost ISR
	 *  - nested > 1  → nested ISR exit
	 */
	if (cpu->nested == 1U) {
		uint32_t id = cpu->id;

		__ASSERT(id < CONFIG_MP_MAX_NUM_CPUS, "Invalid CPU ID");

		/* Restore the original thread SP. */
		return rh850_isr_saved_sp[id];
	}

	/* Nested ISR exit: stay on IRQ stack. */
	return current_sp;
}
