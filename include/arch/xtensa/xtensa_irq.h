/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XTENSA_IRQ_H
#define XTENSA_IRQ_H

#include <xtensa_api.h>
#include <xtensa/xtruntime.h>

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
static ALWAYS_INLINE void _arch_irq_enable(uint32_t irq)
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
static ALWAYS_INLINE void _arch_irq_disable(uint32_t irq)
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

#endif /* XTENSA_IRQ_H */
