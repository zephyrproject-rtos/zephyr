/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_PRIV_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_PRIV_H_

#include <zephyr/kernel.h>

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static inline void flash_page_layout_not_implemented(void)
{
	k_panic();
}
#endif

#define SOC_NV_FLASH_COMPAT_NODE(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())

/**
 * Helper to get the soc-nv-flash compatible child node of a flash controller instance.
 * Only works for flash controllers with a single soc-nv-flash child node.
 * @param inst: Flash controller instance
 */
#define SOC_NV_FLASH_CHILD_NODE(inst) \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SOC_NV_FLASH_COMPAT_NODE)

#endif
