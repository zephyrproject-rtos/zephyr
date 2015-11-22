/* sw_isr_table.c - Software ISR table for quark_se-arc BSP */

/*
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
 * This contains the ISR table meant to be used for ISRs that take a parameter.
 * It is also used when ISRs are to be connected at runtime, and in this case
 * provides a table that is filled with _SpuriousIRQ bindings.
 */

#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>

extern void _irq_spurious(void *arg);

#if defined(CONFIG_SW_ISR_TABLE_DYNAMIC)

_IsrTableEntry_t __isr_table_section _sw_isr_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)].arg = (void *)0xABAD1DEA,
	[0 ...(CONFIG_NUM_IRQS - 1)].isr = _irq_spurious
};

#else
#if defined(CONFIG_SW_ISR_TABLE)
#if !defined(CONFIG_SW_ISR_TABLE_STATIC_CUSTOM)

/* placeholders: fill with real ISRs */

#endif
#endif
#endif
