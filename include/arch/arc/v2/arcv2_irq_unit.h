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

#ifdef __cplusplus
extern "C" {
#endif

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

/*
 * @brief Set interrupt sensitivity
 *
 * Set the sensitivity of the specified interrupt to either
 * _ARC_V2_INT_LEVEL or _ARC_V2_INT_PULSE. Level interrupts will remain
 * asserted until the interrupt handler clears the interrupt at the peripheral.
 * Pulse interrupts self-clear as the interrupt handler is entered.
 *
 * @return N/A
 */

static inline void _arc_v2_irq_unit_sensitivity_set(int irq, int s)
{
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, s);
}

/*
 * @brief Sets an IRQ line to level/pulse trigger
 *
 * Sets the IRQ line <irq> to trigger an interrupt based on the level or the
 * edge of the signal. Valid values for <trigger> are _ARC_V2_INT_LEVEL and
 * _ARC_V2_INT_PULSE.
 *
 * @return N/A
 */

void _arc_v2_irq_unit_trigger_set(int irq, unsigned int trigger);

/*
 * @brief Returns an IRQ line trigger type
 *
 * Gets the IRQ line <irq> trigger type.
 * Valid values for <trigger> are _ARC_V2_INT_LEVEL and _ARC_V2_INT_PULSE.
 *
 * @return N/A
 */

unsigned int _arc_v2_irq_unit_trigger_get(int irq);

/*
 * @brief Send EOI signal to interrupt unit
 *
 * This routine sends an EOI (End Of Interrupt) signal to the interrupt unit
 * to clear a pulse-triggered interrupt.
 *
 * Interrupts must be locked or the ISR operating at P0 when invoking this
 * function.
 *
 * @return N/A
 */

void _arc_v2_irq_unit_int_eoi(int irq);

/*
 * @brief Initialize the interrupt unit device driver
 *
 * Initializes the interrupt unit device driver and the device
 * itself.
 *
 * Interrupts are still locked at this point, so there is no need to protect
 * the window between a write to IRQ_SELECT and subsequent writes to the
 * selected IRQ's registers.
 *
 * @return N/A
 */

void _arc_v2_irq_unit_init(void);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARC_V2_IRQ_UNIT__H */
