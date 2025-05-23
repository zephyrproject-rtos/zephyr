/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fatal.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include <zephyr/irq.h>

#define IR_BASE_ADDRESS  DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IR)
#define IER_BASE_ADDRESS DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IER)
#define IPR_BASE_ADDRESS DT_REG_ADDR_BY_NAME(DT_NODELABEL(icu), IPR)

#define NUM_IRQS_PER_REG  8
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)
#define REG(addr)         *((uint8_t *)(addr))

/**
 * @brief Enable an IRQ by setting the corresponding IEN bit.
 *
 * Note that this will have no effect for IRQs 0-15 as the
 * Renesas rx chip ignores write operations on the corresponding
 * Registers
 *
 * @param irq    interrupt to enable (16-255)
 */
void arch_irq_enable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to enable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to enable reserved interrupt (%u)",
		 irq);

	uint32_t key = irq_lock();

	/* reset interrupt before activating */
	WRITE_BIT(REG(IR_BASE_ADDRESS + irq), 0, false);
	WRITE_BIT(REG(IER_BASE_ADDRESS + REG_FROM_IRQ(irq)), BIT_FROM_IRQ(irq), true);
	irq_unlock(key);
}

/**
 * @brief Disable an IRQ by clearing the corresponding IEN bit.
 *
 * Note that this will have no effect for IRQs 0-15 as the
 * Renesas rx chip ignores write operations on the corresponding
 * Registers.
 *
 * @param irq    interrupt to disable (16-255)
 */
void arch_irq_disable(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "trying to disable invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "trying to disable reserved interrupt (%u)",
		 irq);

	uint32_t key = irq_lock();

	WRITE_BIT(REG(IER_BASE_ADDRESS + REG_FROM_IRQ(irq)), BIT_FROM_IRQ(irq), false);
	irq_unlock(key);
}

/**
 * @brief Determine if an IRQ is enabled by reading the corresponding IEN bit.
 *
 * @param irq   interrupt number
 *
 * @return      true if the interrupt is enabled
 */
int arch_irq_is_enabled(unsigned int irq)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "is_enabled on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "is_enabled on reserved interrupt (%u)", irq);

	return (REG(IER_BASE_ADDRESS + REG_FROM_IRQ(irq)) & BIT(BIT_FROM_IRQ(irq))) != 0;
}

/*
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * @return N/A
 */

void z_irq_spurious(const void *unused)
{
	ARG_UNUSED(unused);
	z_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

/*
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * Higher values take priority over lower values.
 *
 * @return N/A
 */

void z_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "irq_priority_set on invalid interrupt (%u)", irq);
	__ASSERT(irq >= CONFIG_GEN_IRQ_START_VECTOR, "irq_priority_set on reserved interrupt (%u)",
		 irq);
	__ASSERT(prio < CONFIG_NUM_IRQ_PRIO_LEVELS, "invalid priority (%u) for interrupt %u", prio,
		 irq);

	uint32_t key = irq_lock();

	if (irq >= 34) {
		/* for interrupts >= 34, the IPR is regular */
		REG(IPR_BASE_ADDRESS + irq) = prio;
	} else {
		switch (irq) {
		/* 0-15: no IPR */
		case 16:
		/* 17: no IPR */
		case 18:
			REG(IPR_BASE_ADDRESS) = prio;
			break;
		/* 19,20: no IPR */
		case 21:
			REG(IPR_BASE_ADDRESS + 1) = prio;
			break;
		/* 22: no IPR */
		case 23:
			REG(IPR_BASE_ADDRESS + 2) = prio;
			break;
		/* 24,25: no IPR */
		case 26:
		case 27:
			REG(IPR_BASE_ADDRESS + 3) = prio;
			break;
		case 28:
			REG(IPR_BASE_ADDRESS + 4) = prio;
			break;
		case 29:
			REG(IPR_BASE_ADDRESS + 5) = prio;
			break;
		case 30:
			REG(IPR_BASE_ADDRESS + 6) = prio;
			break;
		case 31:
			REG(IPR_BASE_ADDRESS + 7) = prio;
			break;
			/* 32,33: no IPR */
		}
	}
	irq_unlock(key);
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
/**
 * @brief connect a callback function to an interrupt at runtime
 *
 * @param irq          interrupt number
 * @param priority     priority of the interrupt
 * @param routine      routine to call when the interrupt is triggered
 * @param parameter    parameter to supply to the routine on call
 * @param flags        flags for the interrupt
 *
 * @return             the interrupt number
 */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	z_isr_install(irq, routine, parameter);
	z_irq_priority_set(irq, priority, flags);
	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
