/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_API_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_API_H_

#include <xtensa/hal.h>
#include "xtensa_rtos.h"
#include "xtensa_context.h"

/*
 * Call this function to enable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be enabled.
 */
static inline void z_xt_ints_on(unsigned int mask)
{
	int val;

	__asm__ volatile("rsr.intenable %0" : "=r"(val));
	val |= mask;
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(val));
}


/*
 * Call this function to disable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be disabled.
 */
static inline void z_xt_ints_off(unsigned int mask)
{
	int val;

	__asm__ volatile("rsr.intenable %0" : "=r"(val));
	val &= ~mask;
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(val));
}

/*
 * Call this function to set the specified (s/w) interrupt.
 */
static inline void z_xt_set_intset(unsigned int arg)
{
	xthal_set_intset(arg);
}


/* Call this function to clear the specified (s/w or edge-triggered)
 * interrupt.
 */
static inline void _xt_set_intclear(unsigned int arg)
{
	xthal_set_intclear(arg);
}

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_API_H_ */

