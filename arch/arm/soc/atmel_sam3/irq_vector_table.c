/*
 * Copyright (c) 2016 Intel Corporation.
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
