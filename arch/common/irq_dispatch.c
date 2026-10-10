/*
 * Copyright (c) 2026 Picoheart Inc.
 * Copyright (c) 2026 Zhan Gao <gaozhan.9426@picoheart.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared dispatch wrapper for _sw_isr_table.
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/iterable_sections.h>

#include "irq_dispatch.h"

/*
 * Called from level-1 CPU trap dispatch, and from aggregator drivers
 * for level-2 dispatch under CONFIG_MULTI_LEVEL_INTERRUPTS, in place
 * of their usual direct _sw_isr_table indexing.  idx is always the
 * absolute slot index into _sw_isr_table.
 */
void irq_dispatch(unsigned int idx)
{
	struct _isr_table_entry *ite;

	if (idx >= IRQ_TABLE_SIZE) {
		return;
	}

	ite = &_sw_isr_table[idx];

	if (ite->isr != NULL) {
		ite->isr(ite->arg);
	}

	STRUCT_SECTION_FOREACH(irq_dispatch_hook, hook) {
		hook->account(idx);
	}
}
