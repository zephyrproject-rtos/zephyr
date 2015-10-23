/* cortex_m/irq.h - Cortex-M public interrupt handling */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
 * ARM-specific nanokernel interrupt handling interface. Included by ARM/arch.h.
 */

#ifndef _ARCH_ARM_CORTEXM_IRQ_H_
#define _ARCH_ARM_CORTEXM_IRQ_H_

#include <arch/arm/cortex_m/nvic.h>
#include <sw_isr_table.h>

#ifdef _ASMLANGUAGE
GTEXT(_IntExit);
GTEXT(irq_connect)
GTEXT(irq_enable)
GTEXT(irq_disable)
#else
extern int irq_connect(unsigned int irq,
			     unsigned int prio,
			     void (*isr)(void *arg),
			     void *arg);

extern void irq_enable(unsigned int irq);
extern void irq_disable(unsigned int irq);

extern void _IntExit(void);

/* macros convert value of it's argument to a string */
#define DO_TOSTR(s) #s
#define TOSTR(s) DO_TOSTR(s)

/* concatenate the values of the arguments into one */
#define DO_CONCAT(x, y) x ## y
#define CONCAT(x, y) DO_CONCAT(x, y)

/**
 *
 * @brief Connect a routine to interrupt number
 *
 * For the device @a device associates IRQ number @a irq with priority
 * @a priority with the interrupt routine @a isr, that receives parameter
 * @a parameter
 *
 * @return N/A
 *
 */
#define IRQ_CONNECT_STATIC(device, irq, priority, isr, parameter)	\
	const unsigned int _##device##_int_priority = (priority);	\
	struct _IsrTableEntry CONCAT(_isr_irq, irq)			\
	__attribute__ ((section(TOSTR(CONCAT(.gnu.linkonce.isr_irq, irq))))) = \
	{parameter, isr}

/* internal routine documented in C file, needed by IRQ_CONFIG macro */
extern void _irq_priority_set(unsigned int irq, unsigned int prio);

/**
 *
 * @brief Configure interrupt for the device
 *
 * For the given device do the necessary configuration steps.
 * For ARM platform, set the interrupt priority
 *
 * @param device Device to configure
 * @param irq IRQ number
 * @param priority IRQ priority (unused on this architecture)
 * @return N/A
 *
 */
#define IRQ_CONFIG(device, irq, priority) \
	_irq_priority_set(irq, _##device##_int_priority)

#endif /* _ASMLANGUAGE */

#endif /* _ARCH_ARM_CORTEXM_IRQ_H_ */
