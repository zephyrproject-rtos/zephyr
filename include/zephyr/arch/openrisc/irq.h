/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_OR1K_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_OR1K_IRQ_H_

#include <openrisc/openriscregs.h>

#define SPR_SR_IRQ_MASK		(SPR_SR_IEE | SPR_SR_TEE)

#define OR1K_PIC_NUM_INTERRUPTS	32

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS

/* for _soc_irq_*() */
#include <soc.h>

#ifdef CONFIG_2ND_LEVEL_INTERRUPTS
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
#define CONFIG_NUM_IRQS (OR1K_PIC_NUM_INTERRUPTS +\
			(CONFIG_NUM_2ND_LEVEL_AGGREGATORS +\
			CONFIG_NUM_3RD_LEVEL_AGGREGATORS) *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#else
#define CONFIG_NUM_IRQS (OR1K_PIC_NUM_INTERRUPTS +\
			CONFIG_NUM_2ND_LEVEL_AGGREGATORS *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
#else
#define CONFIG_NUM_IRQS OR1K_PIC_NUM_INTERRUPTS
#endif /* CONFIG_2ND_LEVEL_INTERRUPTS */

void z_soc_irq_init(void);
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);

#define arch_irq_enable(irq)	z_soc_irq_enable(irq)
#define arch_irq_disable(irq)	z_soc_irq_disable(irq)

#define arch_irq_is_enabled(irq)	z_soc_irq_is_enabled(irq)

#ifdef CONFIG_DYNAMIC_INTERRUPTS
extern int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
				     void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags);
#endif

#else

#define CONFIG_NUM_IRQS OR1K_PIC_NUM_INTERRUPTS

#define arch_irq_enable(irq)	openrisc_irq_enable(irq)
#define arch_irq_disable(irq)	openrisc_irq_disable(irq)

#define arch_irq_is_enabled(irq)	openrisc_irq_is_enabled(irq)

#endif

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	const uint32_t sr = openrisc_read_spr(SPR_SR);

	openrisc_write_spr(SPR_SR, sr & ~SPR_SR_IRQ_MASK);
	return (sr & SPR_SR_IRQ_MASK) ? 1 : 0;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	const uint32_t sr = openrisc_read_spr(SPR_SR);

	openrisc_write_spr(SPR_SR, key ? (sr | SPR_SR_IRQ_MASK) : (sr & ~SPR_SR_IRQ_MASK));
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

/**
 * @brief Enable interrupt on OpenRISC core.
 *
 * @param irq Interrupt to be enabled.
 */
static ALWAYS_INLINE void arch_irq_enable(unsigned int irq)
{
	const unsigned int key = arch_irq_lock();

	openrisc_write_spr(SPR_PICMR, openrisc_read_spr(SPR_PICMR) | BIT(irq));
	arch_irq_unlock(key);
}

/**
 * @brief Disable interrupt on OpenRISC core.
 *
 * @param irq Interrupt to be disabled.
 */
static ALWAYS_INLINE void arch_irq_disable(unsigned int irq)
{
	const unsigned int key = arch_irq_lock();

	openrisc_write_spr(SPR_PICMR, openrisc_read_spr(SPR_PICMR) & ~BIT(irq));
	arch_irq_unlock(key);
};

static ALWAYS_INLINE int arch_irq_is_enabled(unsigned int irq)
{
	return (openrisc_read_spr(SPR_PICMR) & BIT(irq)) != 0;
}

#endif /* ZEPHYR_INCLUDE_ARCH_OR1K_IRQ_H_ */
