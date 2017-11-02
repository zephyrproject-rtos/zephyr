/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Abstraction layer for x86 interrupt controllers
 *
 * Most x86 just support APIC. However we are starting to see design
 * variants such as MVIC or APICs with reduced feature sets. This
 * interface provides a layer of abstraction between the core arch code
 * and the interrupt controller implementation for x86
 */

#ifndef IRQ_CONTROLLER_H
#define IRQ_CONTROLLER_H

#ifdef CONFIG_MVIC
#include <drivers/mvic.h>
#else
#include <drivers/sysapic.h>
#endif

/* Triggering flags abstraction layer.
 * If a particular set of triggers is not supported, leave undefined
 */
#define IRQ_TRIGGER_EDGE	_IRQ_TRIGGER_EDGE
#define IRQ_TRIGGER_LEVEL	_IRQ_TRIGGER_LEVEL

#define IRQ_POLARITY_HIGH	_IRQ_POLARITY_HIGH
#define IRQ_POLARITY_LOW	_IRQ_POLARITY_LOW

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#if CONFIG_X86_FIXED_IRQ_MAPPING
/**
 * @brief Return fixed mapping for an IRQ
 *
 * @param irq Interrupt line
 * @return Vector this interrupt has been assigned to
 */
#define _IRQ_CONTROLLER_VECTOR_MAPPING(irq) \
	__IRQ_CONTROLLER_VECTOR_MAPPING(irq)
#endif

/**
 *
 * @brief Program an interrupt
 *
 * This function sets the triggering options for an IRQ and also associates
 * the IRQ with its vector in the IDT. This does not enable the interrupt
 * line, it will be left masked.
 *
 * The flags parameter is limited to the IRQ_TRIGGER_* defines above. In
 * addition, not all interrupt controllers support all flags.
 *
 * @param irq Virtualized IRQ
 * @param vector Vector Number
 * @param flags Interrupt flags
 *
 * @returns: N/A
 */
static inline void _irq_controller_irq_config(unsigned int vector,
					      unsigned int irq, u32_t flags)
{
	__irq_controller_irq_config(vector, irq, flags);
}

/**
 * @brief Return the vector of the currently in-service ISR
 *
 * This function should be called in interrupt context.
 * It is not expected for this function to reveal the identity of
 * vectors triggered by a CPU exception or 'int' instruction.
 *
 * @return the vector of the interrupt that is currently being processed, or
 * -1 if this can't be determined
 */
static inline int _irq_controller_isr_vector_get(void)
{
	return __irq_controller_isr_vector_get();
}


static inline void _irq_controller_eoi(void)
{
	__irq_controller_eoi();
}

#else /* _ASMLANGUAGE */

/**
 * @brief Send EOI to the interrupt controller
 *
 * This macro is used by the core interrupt handling code. Interrupts
 * will be locked when this gets called and will not be unlocked until
 * 'iret' has been issued.
 *
 * This macro is used in exactly one spot in intStub.S, in _IntExitWithEoi.
 * At the time this is called, implementations are free to use the caller-
 * saved registers eax, edx, ecx for their own purposes with impunity but
 * need to preserve all callee-saved registers.
 */
.macro _irq_controller_eoi_macro
	__irq_controller_eoi_macro
.endm

#endif /* _ASMLANGUAGE */

#endif /* IRQ_CONTROLLER_H */
