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
 * @file
 * @brief Stubs for IRQ part of vector table
 *
 * When GDB is enabled, the static IRQ vector table needs to install the
 * _irq_vector_table_entry_with_gdb_stub stub to do some work before calling the
 * user-installed ISRs.
 */

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_GDB_INFO) && !defined(CONFIG_SW_ISR_TABLE)

vth __gdb_stub_irq_vector_table _irq_vector_table_with_gdb_stub[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _irq_vector_table_entry_with_gdb_stub
};

#endif /* CONFIG_GDB_INFO && !CONFIG_SW_ISR_TABLE */
