/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>

void z_isr_invoke(uint32_t irq_index)
{
	struct _isr_table_entry entry;

	z_isr_get_entry(irq_index, &entry);
	entry.isr(entry.arg);
}
