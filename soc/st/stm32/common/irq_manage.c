/*
 * Copyright (c) 2024, Etienne Raquin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 SOC specific interrupt management
 *
 *
 * From arch/arm/core/cortex_m/irq_manage.c
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>

#define NUM_IRQS_PER_REG  32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
static const struct device *dev_syscfg = DEVICE_DT_GET(DT_INST(0, st_stm32_syscfg));
#endif

void z_soc_irq_enable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && (irq_get_level(irq) == 2)) {
		irq_enable_next_level(dev_syscfg, irq);
		return;
	}

	NVIC_EnableIRQ((IRQn_Type)irq);
}

void z_soc_irq_disable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && (irq_get_level(irq) == 2)) {
		irq_disable_next_level(dev_syscfg, irq);
		return;
	}

	NVIC_DisableIRQ((IRQn_Type)irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && (irq_get_level(irq) == 2)) {
		return irq_line_is_enabled_next_level(dev_syscfg, irq);
	}

	return NVIC->ISER[REG_FROM_IRQ(irq)] & BIT(BIT_FROM_IRQ(irq));
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved.
 */
void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS) && (irq_get_level(irq) == 2)) {
		/* Set higher level */
		irq = irq_parent_level_2(irq);
	}

	/* The kernel may reserve some of the highest priority levels.
	 * So we offset the requested priority level with the number
	 * of priority levels reserved by the kernel.
	 */

	/* If we have zero latency interrupts, those interrupts will
	 * run at a priority level which is not masked by irq_lock().
	 * Our policy is to express priority levels with special properties
	 * via flags
	 */
	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) && (flags & IRQ_ZERO_LATENCY)) {
		if (ZERO_LATENCY_LEVELS == 1) {
			prio = _EXC_ZERO_LATENCY_IRQS_PRIO;
		} else {
			/* Use caller supplied prio level as-is */
		}
	} else {
		prio += _IRQ_PRIO_OFFSET;
	}

	/* The last priority level is also used by PendSV exception, but
	 * allow other interrupts to use the same level, even if it ends up
	 * affecting performance (can still be useful on systems with a
	 * reduced set of priorities, like Cortex-M0/M0+).
	 */
	__ASSERT(prio <= (BIT(NUM_IRQ_PRIO_BITS) - 1),
		 "invalid priority %d for %d irq! values must be less than %lu\n",
		 prio - _IRQ_PRIO_OFFSET, irq, BIT(NUM_IRQ_PRIO_BITS) - (_IRQ_PRIO_OFFSET));
	NVIC_SetPriority((IRQn_Type)irq, prio);
}

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their priority set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 */
void z_soc_irq_init(void)
{
	int irq = 0;

	for (; irq < CONFIG_NUM_IRQS; irq++) {
		NVIC_SetPriority((IRQn_Type)irq, _IRQ_PRIO_OFFSET);
	}
}

unsigned int z_soc_irq_get_active(void)
{
	return __get_IPSR();
}

void z_soc_irq_eoi(unsigned int irq)
{
	(void)irq;
}
