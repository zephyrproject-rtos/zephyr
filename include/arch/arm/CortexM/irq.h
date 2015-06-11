/* CortexM/irq.h - Cortex-M public interrupt handling */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
ARM-specific nanokernel interrupt handling interface. Included by ARM/arch.h.
*/

#ifndef _ARCH_ARM_CORTEXM_IRQ_H_
#define _ARCH_ARM_CORTEXM_IRQ_H_

#include <arch/arm/CortexM/nvic.h>
#include <sw_isr_table.h>

#ifdef _ASMLANGUAGE
GTEXT(_IntExit);
GTEXT(irq_lock)
GTEXT(irq_unlock)
GTEXT(irq_handler_set)
GTEXT(irq_connect)
GTEXT(irq_disconnect)
GTEXT(irq_enable)
GTEXT(irq_disable)
GTEXT(irq_priority_set)
#else
extern int irq_lock(void);
extern void irq_unlock(int key);

extern void irq_handler_set(unsigned int irq,
				 void (*old)(void *arg),
				 void (*new)(void *arg),
				 void *arg);
extern int irq_connect(unsigned int irq,
			     unsigned int prio,
			     void (*isr)(void *arg),
			     void *arg);
extern void irq_disconnect(unsigned int irq);

extern void irq_enable(unsigned int irq);
extern void irq_disable(unsigned int irq);

extern void irq_priority_set(unsigned int irq, unsigned int prio);

extern void _IntExit(void);

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/*******************************************************************************
 *
 * IRQ_CONNECT_STATIC - connect a routine to interrupt number
 *
 * For the device <device> associates IRQ number <irq> with priority
 * <priority> with the interrupt routine <isr>, that receives parameter
 * <parameter>
 *
 * RETURNS: N/A
 *
 */
#define IRQ_CONNECT_STATIC(device, irq, priority, isr, parameter)	\
	const unsigned int _##device##_int_priority = (priority);		\
	struct _IsrTableEntry CONCAT(_isr_irq, irq)			\
	__attribute__ ((section (TOSTR(CONCAT(.gnu.linkonce.isr_irq, irq))))) = \
	{parameter, isr}

/*******************************************************************************
 *
 * IRQ_CONFIG - configure interrupt for the device
 *
 * For the given device do the neccessary configuration steps.
 * Fpr ARM platform, set the interrupt priority
 *
 * RETURNS: N/A
 *
 */
#define IRQ_CONFIG(device, irq) irq_priority_set(irq, _##device##_int_priority)

#endif /* _ASMLANGUAGE */

#endif /* _ARCH_ARM_CORTEXM_IRQ_H_ */
