/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __XTENSA_API_H__
#define __XTENSA_API_H__

#include <xtensa/hal.h>
#include "xtensa_rtos.h"
#include "xtensa_context.h"

/*
 * Call this function to enable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be enabled.
 */
extern void _xt_ints_on(unsigned int mask);


/*
 * Call this function to disable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be disabled.
 */
extern void _xt_ints_off(unsigned int mask);


/*
 * Call this function to set the specified (s/w) interrupt.
 */
static inline void _xt_set_intset(unsigned int arg)
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


#endif /* __XTENSA_API_H__ */

