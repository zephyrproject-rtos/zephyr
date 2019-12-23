/* arcv2_irq_unit.h - ARCv2 Interrupt Unit device driver */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_ARCV2_IRQ_UNIT_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_ARCV2_IRQ_UNIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* configuration flags for interrupt unit */
#define _ARC_V2_INT_PRIO_MASK 0xf
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

static ALWAYS_INLINE
void z_arc_v2_irq_unit_irq_enable_set(
	int irq,
	unsigned char enable
	)
{
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_ENABLE, enable);
}

/*
 * @brief Enable interrupt
 *
 * Enables the specified interrupt
 *
 * @return N/A
 */

static ALWAYS_INLINE
void z_arc_v2_irq_unit_int_enable(int irq)
{
	z_arc_v2_irq_unit_irq_enable_set(irq, _ARC_V2_INT_ENABLE);
}

/*
 * @brief Disable interrupt
 *
 * Disables the specified interrupt
 *
 * @return N/A
 */

static ALWAYS_INLINE
void z_arc_v2_irq_unit_int_disable(int irq)
{
	z_arc_v2_irq_unit_irq_enable_set(irq, _ARC_V2_INT_DISABLE);
}

/*
 * @brief Poll the enable status of interrupt
 *
 * Polls the enable status of the specified interrupt
 *
 * @return 1 enabled, 0 disabled
 */

static ALWAYS_INLINE
bool z_arc_v2_irq_unit_int_enabled(int irq)
{
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	return z_arc_v2_aux_reg_read(_ARC_V2_IRQ_ENABLE) & 0x1;
}


/*
 * @brief Set interrupt priority
 *
 * Set the priority of the specified interrupt
 *
 * @return N/A
 */

static ALWAYS_INLINE
void z_arc_v2_irq_unit_prio_set(int irq, unsigned char prio)
{

	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
#if defined(CONFIG_ARC_SECURE_FIRMWARE)
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
	(z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY) & (~_ARC_V2_INT_PRIO_MASK))
	| prio);
#else
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY, prio);
#endif
}

#if defined(CONFIG_ARC_SECURE_FIRMWARE)
static ALWAYS_INLINE
void z_arc_v2_irq_uinit_secure_set(int irq, bool secure)
{
		z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);

		if (secure) {
			z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
			z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY)  |
			_ARC_V2_IRQ_PRIORITY_SECURE);
		} else {
			z_arc_v2_aux_reg_write(_ARC_V2_IRQ_PRIORITY,
			z_arc_v2_aux_reg_read(_ARC_V2_IRQ_PRIORITY) &
			_ARC_V2_INT_PRIO_MASK);
		}
}
#endif

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

static ALWAYS_INLINE
void z_arc_v2_irq_unit_sensitivity_set(int irq, int s)
{
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_SELECT, irq);
	z_arc_v2_aux_reg_write(_ARC_V2_IRQ_TRIGGER, s);
}

/*
 * @brief Check whether processor in interrupt/exception state
 *
 * Check whether processor in interrupt/exception state
 *
 * @return N/A
 */
static ALWAYS_INLINE
bool z_arc_v2_irq_unit_is_in_isr(void)
{
	u32_t act = z_arc_v2_aux_reg_read(_ARC_V2_AUX_IRQ_ACT);

	/* in exception ?*/
	if (z_arc_v2_aux_reg_read(_ARC_V2_STATUS32) & _ARC_V2_STATUS32_AE) {
		return true;
	}

	return ((act & 0xffff) != 0U);
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

void z_arc_v2_irq_unit_trigger_set(int irq, unsigned int trigger);

/*
 * @brief Returns an IRQ line trigger type
 *
 * Gets the IRQ line <irq> trigger type.
 * Valid values for <trigger> are _ARC_V2_INT_LEVEL and _ARC_V2_INT_PULSE.
 *
 * @return N/A
 */

unsigned int z_arc_v2_irq_unit_trigger_get(int irq);

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

void z_arc_v2_irq_unit_int_eoi(int irq);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_ARCV2_IRQ_UNIT_H_ */
