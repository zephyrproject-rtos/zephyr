/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include "ull_scan_types.h"
#include "lll_scan.h"

#define BT_CTLR_SCAN_MAX 1
static struct ll_scan_set ll_scan[BT_CTLR_SCAN_MAX];

u8_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			u8_t own_addr_type, u8_t filter_policy)
{
	struct ll_scan_set *scan;

	scan = ull_scan_is_disabled_get(0);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	scan->own_addr_type = own_addr_type;

	return 0;
}

struct ll_scan_set *ull_scan_set_get(u16_t handle)
{
	if (handle >= BT_CTLR_SCAN_MAX) {
		return NULL;
	}

	return &ll_scan[handle];
}

struct ll_scan_set *ull_scan_is_enabled_get(u16_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || !scan->is_enabled) {
		return NULL;
	}

	return scan;
}

struct ll_scan_set *ull_scan_is_disabled_get(u16_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || scan->is_enabled) {
		return NULL;
	}

	return scan;
}
