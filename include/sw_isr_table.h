/*
 * Copyright (c) 2014, Wind River Systems, Inc.
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
 * @brief Software-managed ISR table
 *
 * Data types for a software-managed ISR table, with a parameter per-ISR.
 */

#ifndef _SW_ISR_TABLE__H_
#define _SW_ISR_TABLE__H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_ASMLANGUAGE)
/*
 * Note the order: arg first, then ISR. This allows a table entry to be
 * loaded arg -> r0, isr -> r3 in _isr_wrapper with one ldmia instruction,
 * on ARM Cortex-M (Thumb2).
 */
struct _IsrTableEntry {
	void *arg;
	void (*isr)(void *);
};

typedef struct _IsrTableEntry _IsrTableEntry_t;

#ifdef CONFIG_ARC
extern _IsrTableEntry_t _sw_isr_table[CONFIG_NUM_IRQS - 16];
#elif CONFIG_NIOS2
extern _IsrTableEntry_t _sw_isr_table[NIOS2_NIRQ];
#else
extern _IsrTableEntry_t _sw_isr_table[CONFIG_NUM_IRQS];
#endif

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SW_ISR_TABLE__H_ */
