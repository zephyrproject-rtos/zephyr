/* arcv2_irq_unit.c - ARCv2 Interrupt Unit device driver */

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

/*
 * DESCRIPTION
 * The ARCv2 interrupt unit has 16 allocated exceptions associated with
 * vectors 0 to 15 and 240 interrupts associated with vectors 16 to 255.
 * The interrupt unit is optional in the ARCv2-based processors. When
 * building a processor, you can configure the processor to include an
 * interrupt unit. The ARCv2 interrupt unit is highly programmable.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <board.h>
extern void *_VectorTable;

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

void _arc_v2_irq_unit_init(void)
{
	int irq; /* the interrupt index */

	for (irq = 16; irq < 256; irq++) {
		_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
		_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY, 1);
		_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, _ARC_V2_INT_DISABLE);
		_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, _ARC_V2_INT_LEVEL);
	}
	_arc_v2_aux_reg_write (_ARC_V2_IRQ_VECT_BASE, &_VectorTable);
}

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

void _arc_v2_irq_unit_int_eoi(int irq)
{
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_PULSE_CANCEL, 1);
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

void _arc_v2_irq_unit_trigger_set(int irq, unsigned int trigger)
{
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, trigger);
}
