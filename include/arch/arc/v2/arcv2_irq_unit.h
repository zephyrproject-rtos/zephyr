/* arcv2_irq_unit.h - ARCv2 Interrupt Unit device driver */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

#ifndef _ARC_V2_IRQ_UNIT__H
#define _ARC_V2_IRQ_UNIT__H

/* configuration flags for interrupt unit */

#define _ARC_V2_INT_DISABLE 0
#define _ARC_V2_INT_ENABLE 1

#define _ARC_V2_INT_LEVEL 0
#define _ARC_V2_INT_PULSE 1

#ifndef _ASMLANGUAGE

/*
 * WARNING:
 *
 * All APIs provided by this file must be invoked with INTERRUPTS LOCKED. The
 * APIs themselves are writing the IRQ_SELECT, selecting which IRQ's registers
 * it wants to write to, then write to them: THIS IS NOT AN ATOMIC OPERATION.
 *
 * Not locking the interrupts inside of the APIs allows a caller to:
 *
 * - lock interrupts
 * - call many of these APIs
 * - unlock interrupts
 *
 * thus being more efficient then if the APIs themselves would lock
 * interrupts.
 */

/*
 * @brief Enable/disable interrupt
 *
 * Enables or disables the specified interrupt
 *
 * @return N/A
 */

static inline void _arc_v2_irq_unit_irq_enable_set(
	int irq,
	unsigned char enable
	)
{
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, enable);
}

/*
 * @brief Enable interrupt
 *
 * Enables the specified interrupt
 *
 * @return N/A
 */

static inline void _arc_v2_irq_unit_int_enable(int irq)
{
	_arc_v2_irq_unit_irq_enable_set(irq, _ARC_V2_INT_ENABLE);
}

/*
 * @brief Disable interrupt
 *
 * Disables the specified interrupt
 *
 * @return N/A
 */

static inline void _arc_v2_irq_unit_int_disable(int irq)
{
	_arc_v2_irq_unit_irq_enable_set(irq, _ARC_V2_INT_DISABLE);
}

/*
 * @brief Set interrupt priority
 *
 * Set the priority of the specified interrupt
 *
 * @return N/A
 */

static inline void _arc_v2_irq_unit_prio_set(int irq, unsigned char prio)
{
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY, prio);
}

void _arc_v2_irq_unit_int_eoi(int irq);
void _arc_v2_irq_unit_init(void);

#endif /* _ASMLANGUAGE */
#endif /* _ARC_V2_IRQ_UNIT__H */
