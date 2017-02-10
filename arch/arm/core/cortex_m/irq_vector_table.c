/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IRQ part of vector table
 *
 * This file contains the IRQ part of the vector table. It is meant to be used
 * for one of two cases:
 *
 * a) When software-managed ISRs (SW_ISR_TABLE) is enabled, and in that case it
 *    binds _isr_wrapper() to all the IRQ entries in the vector table.
 *
 * b) When the platform is written so that device ISRs are installed directly in
 *    the vector table, they are enumerated here.
 */

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

extern void _isr_wrapper(void);
typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_SW_ISR_TABLE)

vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _isr_wrapper,
};

#elif !defined(CONFIG_IRQ_VECTOR_TABLE_CUSTOM)

extern void _irq_spurious(void);

/* placeholders: fill with real ISRs */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _irq_spurious,
};

#endif /* CONFIG_SW_ISR_TABLE */
