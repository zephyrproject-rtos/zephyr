/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_BEKEN_BK7258_SOC_H_
#define _SOC_BEKEN_BK7258_SOC_H_

#include <cmsis_core_m_defaults.h>

#ifndef _ASMLANGUAGE

/**
 * @brief Open CPU0's gate for a peripheral interrupt.
 *
 * A peripheral interrupt reaches the NVIC only while its bit in the SYS
 * block's per-CPU gate is set, in addition to being enabled in the NVIC.
 * Bits are numbered as the NVIC numbers the interrupts.
 *
 * @param irq NVIC interrupt number, 0 to 63.
 */
void bk7258_irq_gate_enable(unsigned int irq);

#endif /* !_ASMLANGUAGE */

#endif /* _SOC_BEKEN_BK7258_SOC_H_ */
