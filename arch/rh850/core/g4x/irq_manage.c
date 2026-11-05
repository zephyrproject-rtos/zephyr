/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kswap.h>

/* Per-CPU saved SP of the outermost interrupted thread.
 * Index by arch_curr_cpu()->id, which is set in z_init_cpu().
 */
static uintptr_t rh850_isr_saved_sp[CONFIG_MP_MAX_NUM_CPUS];

#ifndef CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER

/* Maximum number of interrupt lines for each controller type, retrieved from device tree */
#define RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES (DT_PROP(DT_NODELABEL(intc1_pe0), num_lines))
#define RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES (DT_PROP(DT_NODELABEL(intc2), num_lines))
#define RH850_IRQ_FEINT_VECTOR_MAX_ENTRIES (DT_PROP(DT_NODELABEL(feinc_pe0), num_lines))
#define RH850_IRQ_FENMI_VECTOR_MAX_ENTRIES (1UL)

/* Combined interrupt vector counts for range validation */
#define RH850_IRQ_INTC_VECTOR_MAX_ENTRIES                                                          \
	(RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES + RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES)

#define RH850_IRQ_INT_VECTOR_MAX_ENTRIES                                                           \
	(RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES + RH850_IRQ_INTC2_VECTOR_MAX_ENTRIES +                 \
	 RH850_IRQ_FEINT_VECTOR_MAX_ENTRIES + RH850_IRQ_FENMI_VECTOR_MAX_ENTRIES)

/* Register address calculation */
#define RH850_IRQ_EIC_REG(intc_eic_base_p, irq_p)   ((uint8_t *)intc_eic_base_p + 0x02U * irq_p)
#define RH850_IRQ_EIBD_REG(intc_eibd_base_p, irq_p) ((uint8_t *)intc_eibd_base_p + 0x04U * irq_p)

/* Device tree register extraction macros for array initialization */
#define RH850_IRQ_GET_EIC_REG(n)      (uint16_t *)DT_REG_ADDR_BY_NAME(n, EIC),
#define RH850_IRQ_GET_EIBD_REG(n)     (uint32_t *)DT_REG_ADDR_BY_NAME(n, EIBD),
#define RH850_IRQ_GET_FEINTMSK_REG(n) (uint32_t *)DT_REG_ADDR_BY_NAME(n, FEINTMSK),

static uint16_t *const intc1_eic_reg[] = {
	DT_FOREACH_CHILD(DT_NODELABEL(intc1), RH850_IRQ_GET_EIC_REG)};

static uint32_t *const intc1_eibd_reg[] = {
	DT_FOREACH_CHILD(DT_NODELABEL(intc1), RH850_IRQ_GET_EIBD_REG)};

static uint16_t *const intc2_eic_reg = (uint16_t *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc2), EIC);

static uint32_t *const intc2_eibd_reg = (uint32_t *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc2), EIBD);

static uint32_t *const feinc_feintmsk_reg[] = {
	DT_FOREACH_CHILD(DT_NODELABEL(feinc), RH850_IRQ_GET_FEINTMSK_REG)};

void rh850_irq_enable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to enable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to enable reserved interrupt (%u)",
		 irq);

	uint32_t key = irq_lock();

	/* Get the current coreID */
	uint8_t coreID;

	__asm__ volatile("stsr 0, %0, 2\n" : "=r"(coreID) : : "memory");

	/* INTC1 */
	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		volatile uint16_t *R_INTC1_PEx_EIC =
			(uint16_t *)RH850_IRQ_EIC_REG(intc1_eic_reg[coreID], irq);

		/* Set bit EITB of INTC1_PEx EICn register to select table reference method */
		WRITE_BIT(*R_INTC1_PEx_EIC, 6, 1);

		/* Reset bit EIMK of INTC1_PEx EICn register to unmask interrupt */
		WRITE_BIT(*R_INTC1_PEx_EIC, 7, 0);
	}

	/* INTC2 */
	else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		volatile uint16_t *INTC2_EIC_REG =
			(uint16_t *)RH850_IRQ_EIC_REG(intc2_eic_reg, irq);

		/* Set bit EITB of INTC2 EICn register to select table reference method */
		WRITE_BIT(*INTC2_EIC_REG, 6, 1);

		/* Reset bit EIMK of INTC2 EICn register to unmask interrupt */
		WRITE_BIT(*INTC2_EIC_REG, 7, 0);

		/* Reset bit PEID[2:0] of INTC2 EIBDn bind interrupt source to PE */
		volatile uint32_t *INTC2_EIBD_REG =
			(uint32_t *)RH850_IRQ_EIBD_REG(intc2_eibd_reg, irq);
		*INTC2_EIBD_REG = (*INTC2_EIBD_REG & ~0x7U) | (coreID & 0x7U);
	}

	/* FEINT */
	else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		*feinc_feintmsk_reg[coreID] &=
			(uint32_t)(~(1 << (irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES)));
	} else {
		/* Nothing to do */
	}

	irq_unlock(key);
}

void rh850_irq_disable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to disable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to disable reserved interrupt (%u)",
		 irq);

	uint32_t key = irq_lock();

	/* Get the current coreID */
	uint8_t coreID;

	__asm__ volatile("stsr 0, %0, 2\n" : "=r"(coreID) : : "memory");

	/* INTC1 */
	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		/* Set bit EIMK of INTC1_PEx EICn register to mask interrupt */
		volatile uint16_t *R_INTC1_PEx_EIC =
			(uint16_t *)RH850_IRQ_EIC_REG(intc1_eic_reg[coreID], irq);
		WRITE_BIT(*R_INTC1_PEx_EIC, 7, 1);
	}

	/* INTC2 */
	else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		/* Set bit EIMK of INTC2 EICn register to mask interrupt */
		volatile uint16_t *INTC2_EIC_REG =
			(uint16_t *)RH850_IRQ_EIC_REG(intc2_eic_reg, irq);
		WRITE_BIT(*INTC2_EIC_REG, 7, 1);
	}

	/* FEINT */
	else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		*feinc_feintmsk_reg[coreID] |=
			(uint32_t)(1 << (irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES));
	} else {
		/* Nothing to do */
	}

	irq_unlock(key);
}

int rh850_irq_is_enabled(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "is_enabled on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "is_enabled on reserved interrupt (%u)", irq);

	/* Get the current coreID */
	uint8_t coreID;

	__asm__ volatile("stsr 0, %0, 2\n" : "=r"(coreID) : : "memory");

	/* INTC1 */
	if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
		volatile uint16_t *R_INTC1_PEx_EIC =
			(uint16_t *)RH850_IRQ_EIC_REG(intc1_eic_reg[coreID], irq);

		/* Read bit EIMK of INTC1_PEx EICn register */
		return !IS_BIT_SET(*R_INTC1_PEx_EIC, 7);
	}

	/* INTC2 */
	else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
		volatile uint16_t *INTC2_EIC_REG =
			(uint16_t *)RH850_IRQ_EIC_REG(intc2_eic_reg, irq);

		/* Read bit EIMK of INTC2 EICn register */
		return !IS_BIT_SET(*INTC2_EIC_REG, 7);
	}

	/* FEINT */
	else if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		return !IS_BIT_SET(*feinc_feintmsk_reg[coreID],
				   (irq - RH850_IRQ_INTC_VECTOR_MAX_ENTRIES));
	} else {
		return 0;
	}
}

void rh850_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "irq_priority_set on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "irq_priority_set on reserved interrupt (%u)",
		 irq);
	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS, "invalid priority (%u) for interrupt %u", prio,
		 irq);

	uint32_t key = irq_lock();

	if (irq < RH850_IRQ_INT_VECTOR_MAX_ENTRIES) {
		/* INTC1 */
		if (irq < RH850_IRQ_INTC1_VECTOR_MAX_ENTRIES) {
			/* Get the current coreID */
			uint8_t coreID;

			__asm__ volatile("stsr 0, %0, 2\n" : "=r"(coreID) : : "memory");

			/* Set bit EICn.EIPn[0:3] */
			volatile uint16_t *R_INTC1_PEx_EIC =
				(uint16_t *)RH850_IRQ_EIC_REG(intc1_eic_reg[coreID], irq);
			*R_INTC1_PEx_EIC = (*R_INTC1_PEx_EIC & ~0xFU) | (prio & 0xFU);
		}

		/* INTC2 */
		else if (irq < RH850_IRQ_INTC_VECTOR_MAX_ENTRIES) {
			/* Set bit EICn.EIPn[0:3] */
			volatile uint16_t *INTC2_EIC_REG =
				(uint16_t *)RH850_IRQ_EIC_REG(intc2_eic_reg, irq);
			*INTC2_EIC_REG = (*INTC2_EIC_REG & ~0xFU) | (prio & 0xFU);
		}

		/* FEINT */
		else {
			/* Nothing to do */
		}
	}

	irq_unlock(key);
}

#endif /* CONFIG_RH850_CUSTOM_INTERRUPT_CONTROLLER */

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
