/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <xtensa_api.h>
#include <kernel_arch_data.h>
#include <misc/__assert.h>
/*
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved: three for various types of exceptions,
 * and possibly one additional to support zero latency interrupts.
 *
 * Valid values are from 1 to 6. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them. ISR
 * installed with priority 0 interrupts cannot make kernel calls.
 *
 * @return N/A
 */

void _irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(prio < XCHAL_EXCM_LEVEL + 1,
		 "invalid priority %d! values must be less than %d\n",
		 prio, XCHAL_EXCM_LEVEL + 1);
	/* TODO: Write code to set priority if this is ever possible on Xtensa */
}
