/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include "hci_err.h"
#include "util/mem.h"
#include "util/memq.h"
#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_scan.h"

#include "ull_scan_types.h"
#include "ull_scan_internal.h"

#define BT_CTLR_SCAN_MAX 1
static struct ll_scan_set ll_scan[BT_CTLR_SCAN_MAX];

uint8_t ll_scan_params_set(uint8_t type, uint16_t interval, uint16_t window, uint8_t own_addr_type,
			   uint8_t filter_policy)
{
	struct ll_scan_set *scan;

	scan = ull_scan_is_disabled_get(0);
	if (!scan) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	scan->own_addr_type = own_addr_type;

	return 0;
}

struct ll_scan_set *ull_scan_set_get(uint8_t handle)
{
	if (handle >= BT_CTLR_SCAN_MAX) {
		return NULL;
	}

	return &ll_scan[handle];
}

struct ll_scan_set *ull_scan_is_enabled_get(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || !scan->is_enabled) {
		return NULL;
	}

	return scan;
}

struct ll_scan_set *ull_scan_is_disabled_get(uint8_t handle)
{
	struct ll_scan_set *scan;

	scan = ull_scan_set_get(handle);
	if (!scan || scan->is_enabled) {
		return NULL;
	}

	return scan;
}

uint8_t ull_scan_enable(struct ll_scan_set *scan)
{
	return 0;
}

uint32_t ull_scan_params_set(struct lll_scan *lll, uint8_t type,
			     uint16_t interval, uint16_t window,
			     uint8_t filter_policy)
{
	return 0;
}

uint8_t ull_scan_disable(uint8_t handle, struct ll_scan_set *scan)
{
	return 0;
}
