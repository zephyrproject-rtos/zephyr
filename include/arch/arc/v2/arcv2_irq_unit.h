/* arcv2_irq_unit.h - ARCv2 Interrupt Unit device driver */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
 * _arc_v2_irq_unit_irq_enable_set - enable/disable interrupt
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
 * _arc_v2_irq_unit_int_enable - enable interrupt
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
 * _arc_v2_irq_unit_int_disable - disable interrupt
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
 * _arc_v2_irq_unit_prio_set - set interrupt priority
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
