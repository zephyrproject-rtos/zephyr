/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
