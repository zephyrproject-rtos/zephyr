/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "resource_table.h"

extern char ram_console[];

#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)

#ifdef CONFIG_RAM_CONSOLE
static volatile struct stm32_resource_table __resource resource_table = {
	.ver = 1,
	.num = 1,
	.offset = {
		offsetof(struct stm32_resource_table, cm_trace),
	},
	.cm_trace = {
		RSC_TRACE,
		(uint32_t)ram_console, CONFIG_RAM_CONSOLE_BUFFER_SIZE + 1, 0,
		"cm4_log",
	},
};
#endif

void resource_table_init(volatile void **table_ptr, int *length)
{
	*table_ptr = &resource_table;
	*length = sizeof(resource_table);
}

