/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief IRQ part of vector table for Quark SE Sensor Subsystem
 *
 * This file contains the IRQ part of the vector table. It is meant to be used
 * for one of two cases:
 *
 * a) When software-managed ISRs (SW_ISR_TABLE) is enabled, and in that case it
 * binds _IsrWrapper() to all the IRQ entries in the vector table.
 *
 * b) When the BSP is written so that device ISRs are installed directly in the
 * vector table, they are enumerated here.
 *
 */

#include <toolchain.h>
#include <sections.h>

extern void _isr_enter(void);
typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_SW_ISR_TABLE)

vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS - 16] = {
	[0 ...(CONFIG_NUM_IRQS - 17)] = _isr_enter
};

#elif !defined(CONFIG_IRQ_VECTOR_TABLE_CUSTOM)

extern void _SpuriousIRQ(void);

/* placeholders: fill with real ISRs */

vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS - 16] = {
	[0 ...(CONFIG_NUM_IRQS - 17)] = _SpuriousIRQ
};

#endif /* CONFIG_SW_ISR_TABLE */
