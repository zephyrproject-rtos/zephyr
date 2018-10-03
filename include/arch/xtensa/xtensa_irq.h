/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_

#include <xtensa_api.h>
#include <xtensa/xtruntime.h>
#include <board.h>

#define CONFIG_GEN_IRQ_START_VECTOR 0

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS

#define CONFIG_NUM_IRQS (XCHAL_NUM_INTERRUPTS +\
			(CONFIG_NUM_2ND_LEVEL_AGGREGATORS +\
			CONFIG_NUM_3RD_LEVEL_AGGREGATORS) *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)

#define _arch_irq_enable(irq)	_soc_irq_enable(irq)
#define _arch_irq_disable(irq)	_soc_irq_disable(irq)

#else

#define CONFIG_NUM_IRQS XCHAL_NUM_INTERRUPTS

#define _arch_irq_enable(irq)	_xtensa_irq_enable(irq)
#define _arch_irq_disable(irq)	_xtensa_irq_disable(irq)

#endif

/**
 *
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * IRQ.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _xtensa_irq_enable(u32_t irq)
{
	_xt_ints_on(1 << irq);
}

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified IRQ.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _xtensa_irq_disable(u32_t irq)
{
	_xt_ints_off(1 << irq);
}

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key = XTOS_SET_INTLEVEL(XCHAL_EXCM_LEVEL);
	return key;
}

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	XTOS_RESTORE_INTLEVEL(key);
}

#include <irq.h>

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_ */
