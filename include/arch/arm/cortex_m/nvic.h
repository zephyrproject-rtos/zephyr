/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nvic.c - ARM CORTEX-M Series Nested Vector Interrupt Controller
 *
 * Provide an interface to the Nested Vectored Interrupt Controller found on
 * ARM Cortex-M processors.
 *
 * The API does not account for all possible usages of the NVIC, only the
 * functionalities needed by the kernel.
 *
 * The same effect can be achieved by directly writing in the registers of the
 * NVIC, with the layout available from scs.h, using the __scs.nvic data
 * structure (or hardcoded values), but these APIs are less error-prone,
 * especially for registers with multiple instances to account for potentially
 * 240 interrupt lines. If access to a missing functionality is needed, this is
 * the way to implement it.
 *
 * Supports up to 240 IRQs and 256 priority levels.
 */

#ifndef _NVIC_H_
#define _NVIC_H_

#include <arch/arm/cortex_m/scs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#if !defined(_ASMLANGUAGE)

#include <stdint.h>
#include <misc/__assert.h>

/**
 *
 * @brief Enable an IRQ
 *
 * Enable IRQ #@a irq, which is equivalent to exception #@a irq+16
 *
 * @param irq IRQ number
 *
 * @return N/A
 */

static inline void _NvicIrqEnable(unsigned int irq)
{
	__scs.nvic.iser[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is enabled
 *
 * Find out if IRQ #@a irq is enabled.
 *
 * @param irq IRQ number
 * @return 1 if IRQ is enabled, 0 otherwise
 */

static inline int _NvicIsIrqEnabled(unsigned int irq)
{
	return __scs.nvic.iser[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Disable an IRQ
 *
 * Disable IRQ #@a irq, which is equivalent to exception #@a irq+16
 * @param irq IRQ number
 * @return N/A
 */

static inline void _NvicIrqDisable(unsigned int irq)
{
	__scs.nvic.icer[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Pend an IRQ
 *
 * Pend IRQ #@a irq, which is equivalent to exception #@a irq+16. CPU will handle
 * the IRQ when interrupts are enabled and/or returning from a higher priority
 * interrupt.
 * @param irq IRQ number
 *
 * @return N/A
 */

static inline void _NvicIrqPend(unsigned int irq)
{
	__scs.nvic.ispr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is pending
 *
 * Find out if IRQ #@a irq is pending
 *
 * @param irq IRQ number
 * @return 1 if IRQ is pending, 0 otherwise
 */

static inline int _NvicIsIrqPending(unsigned int irq)
{
	return __scs.nvic.ispr[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Unpend an IRQ
 *
 * Unpend IRQ #@a irq, which is equivalent to exception #@a irq+16. The previously
 * pending interrupt will be ignored when either unlocking interrupts or
 * returning from a higher priority exception.
 *
 * @param irq IRQ number
 * @return N/A
 */

static inline void _NvicIrqUnpend(unsigned int irq)
{
	__scs.nvic.icpr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Set priority of an IRQ
 *
 * Set priority of IRQ #@a irq to @a prio. There are 256 priority levels.
 *
 * @param irq IRQ number
 * @param prio Priority
 * @return N/A
 */

static inline void _NvicIrqPrioSet(unsigned int irq, uint8_t prio)
{
#if defined(CONFIG_ARMV6_M)
	volatile uint32_t * const ipr = &__scs.nvic.ipr[_PRIO_IP_IDX(irq)];
	*ipr = ((*ipr & ~((uint32_t)0xff << _PRIO_BIT_SHIFT(irq))) |
		((uint32_t)prio << _PRIO_BIT_SHIFT(irq)));
#elif defined(CONFIG_ARMV7_M)
	__scs.nvic.ipr[irq] = prio;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

/**
 *
 * @brief Get priority of an IRQ
 *
 * Get priority of IRQ #@a irq.
 *
 * @param irq IRQ number
 *
 * @return the priority level of the IRQ
 */

static inline uint8_t _NvicIrqPrioGet(unsigned int irq)
{
#if defined(CONFIG_ARMV6_M)
	return (__scs.nvic.ipr[_PRIO_IP_IDX(irq)] >> _PRIO_BIT_SHIFT(irq));
#elif defined(CONFIG_ARMV7_M)
	return __scs.nvic.ipr[irq];
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
}

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _NVIC_H_ */
