/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_VERSATILE_SOC_H_
#define ZEPHYR_SOC_ARM_VERSATILE_SOC_H_

#ifndef _ASMLANGUAGE

/**
 * @brief Raise an interrupt from software
 *
 * Sets the line's bit in the PL190's software interrupt register. The
 * interrupt is taken as soon as the line is enabled and IRQs are unmasked,
 * and the bit is cleared when it is.
 *
 * @param irq Interrupt line, 0 to 31
 */
void z_versatile_irq_trigger(unsigned int irq);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_SOC_ARM_VERSATILE_SOC_H_ */
