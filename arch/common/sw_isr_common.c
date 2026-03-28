/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq.h>
#include <zephyr/sys/__assert.h>

/*
 * Common code for arches that use software ISR tables (CONFIG_GEN_ISR_TABLES)
 */

unsigned int __weak z_get_sw_isr_table_idx(unsigned int irq)
{
	unsigned int table_idx = irq - CONFIG_GEN_IRQ_START_VECTOR;

	__ASSERT_NO_MSG(table_idx < IRQ_TABLE_SIZE);

	return table_idx;
}
