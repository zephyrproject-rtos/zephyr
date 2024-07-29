/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <helpers/nrfx_ram_ctrl.h>

#define _BUILD_MEM_REGION(node_id)		    \
	{.dt_addr = DT_REG_ADDR(DT_PARENT(node_id)),\
	 .dt_size = DT_REG_SIZE(DT_PARENT(node_id))}

struct ret_mem_region {
	uintptr_t dt_addr;
	size_t dt_size;
};

static const struct ret_mem_region ret_mem_regions[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_retained_ram, _BUILD_MEM_REGION)
};

static int retained_mem_nrf_init(void)
{
	const struct ret_mem_region *rmr;

	for (size_t i = 0; i < ARRAY_SIZE(ret_mem_regions); i++) {
		rmr = &ret_mem_regions[i];
		nrfx_ram_ctrl_retention_enable_set((void *)rmr->dt_addr, rmr->dt_size, true);
	}

	return 0;
}

SYS_INIT(retained_mem_nrf_init, PRE_KERNEL_1, 0);
