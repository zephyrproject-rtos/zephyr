/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/retained_mem/nrf_retained_mem.h>

#if defined(CONFIG_NRF_TFM_RAM_CTRL_SERVICE)
/* Non-secure (TF-M) build: MEMCONF is owned by the secure domain, so route
 * retention requests to the secure RAM-control service instead of touching
 * nrfx directly.
 */
#include "tfm_ioctl_core_api.h"
#else
#include <helpers/nrfx_ram_ctrl.h>
#endif

#define _BUILD_MEM_REGION(node_id)		    \
	{.dt_addr = DT_REG_ADDR(DT_PARENT(node_id)),\
	 .dt_size = DT_REG_SIZE(DT_PARENT(node_id))},

struct ret_mem_region {
	uintptr_t dt_addr;
	size_t dt_size;
};

static const struct ret_mem_region ret_mem_regions[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_retained_ram, _BUILD_MEM_REGION)
};

int z_nrf_retained_mem_retention_apply(void)
{
	const struct ret_mem_region *rmr;
	int rc = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ret_mem_regions); i++) {
		rmr = &ret_mem_regions[i];
#if defined(CONFIG_NRF_TFM_RAM_CTRL_SERVICE)
		/* The secure service can reject a region (e.g. outside non-secure
		 * RAM). Keep applying the rest, but report failure to the caller.
		 */
		if (tfm_platform_ram_ctrl_retention_set(rmr->dt_addr, rmr->dt_size, true) !=
		    TFM_PLATFORM_ERR_SUCCESS) {
			rc = -EIO;
		}
#else
		nrfx_ram_ctrl_retention_enable_set((void *)rmr->dt_addr, rmr->dt_size, true);
#endif
	}

	return rc;
}
