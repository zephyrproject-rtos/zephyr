/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_

#include <arch/xtensa/xtensa_api.h>
#include <xtensa/xtruntime.h>

#define CONFIG_GEN_IRQ_START_VECTOR 0

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS

/* for _soc_irq_*() */
#include <soc.h>

#ifdef CONFIG_2ND_LEVEL_INTERRUPTS
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
#define CONFIG_NUM_IRQS (XCHAL_NUM_INTERRUPTS +\
			(CONFIG_NUM_2ND_LEVEL_AGGREGATORS +\
			CONFIG_NUM_3RD_LEVEL_AGGREGATORS) *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#else
#define CONFIG_NUM_IRQS (XCHAL_NUM_INTERRUPTS +\
			CONFIG_NUM_2ND_LEVEL_AGGREGATORS *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#endif
#else
#define CONFIG_NUM_IRQS XCHAL_NUM_INTERRUPTS
#endif

#define arch_irq_enable(irq)	z_soc_irq_enable(irq)
#define arch_irq_disable(irq)	z_soc_irq_disable(irq)

#define arch_irq_is_enabled(irq)	z_soc_irq_is_enabled(irq)

#ifdef CONFIG_DYNAMIC_INTERRUPTS
extern int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
				     void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags);
#endif

#else

#define CONFIG_NUM_IRQS XCHAL_NUM_INTERRUPTS

#define arch_irq_enable(irq)	z_xtensa_irq_enable(irq)
#define arch_irq_disable(irq)	z_xtensa_irq_disable(irq)

#define arch_irq_is_enabled(irq)	z_xtensa_irq_is_enabled(irq)

#endif

static ALWAYS_INLINE void z_xtensa_irq_enable(uint32_t irq)
{
	z_xt_ints_on(1 << irq);
}

static ALWAYS_INLINE void z_xtensa_irq_disable(uint32_t irq)
{
	z_xt_ints_off(1 << irq);
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	XTOS_RESTORE_INTLEVEL(key);
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key & 0xf) == 0; /* INTLEVEL field */
}

extern int z_xtensa_irq_is_enabled(unsigned int irq);

#include <irq.h>

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_ */
