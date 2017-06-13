/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"

#include "pdu.h"
#include "ctrl.h"
#include "ll.h"
#include "ll_filter.h"

#define ADDR_TYPE_ANON 0xFF

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include "common/log.h"

static struct ll_wl wl;

struct ll_wl *ctrl_wl_get(void)
{
	return &wl;
}

u32_t ll_wl_size_get(void)
{
	return WL_SIZE;
}

u32_t ll_wl_clear(void)
{
	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	wl.enable_bitmask = 0;
	wl.addr_type_bitmask = 0;
	wl.anon = 0;

	return 0;
}

u32_t ll_wl_add(bt_addr_le_t *addr)
{
	u8_t index;

	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr->type == ADDR_TYPE_ANON) {
		wl.anon = 1;
		return 0;
	}

	if (wl.enable_bitmask == 0xFF) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	for (index = 0;
	     (wl.enable_bitmask & (1 << index));
	     index++) {
	}
	wl.enable_bitmask |= (1 << index);
	wl.addr_type_bitmask |= ((addr->type & 0x01) << index);
	memcpy(&wl.bdaddr[index][0], addr->a.val, BDADDR_SIZE);

	return 0;
}

u32_t ll_wl_remove(bt_addr_le_t *addr)
{
	u8_t index;

	if (radio_adv_filter_pol_get() || (radio_scan_filter_pol_get() & 0x1)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (addr->type == ADDR_TYPE_ANON) {
		wl.anon = 0;
		return 0;
	}

	if (!wl.enable_bitmask) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	index = 8;
	while (index--) {
		if ((wl.enable_bitmask & BIT(index)) &&
		    (((wl.addr_type_bitmask >> index) & 0x01) ==
		     (addr->type & 0x01)) &&
		    !memcmp(wl.bdaddr[index], addr->a.val, BDADDR_SIZE)) {
			wl.enable_bitmask &= ~BIT(index);
			wl.addr_type_bitmask &= ~BIT(index);
			return 0;
		}
	}

	return BT_HCI_ERR_INVALID_PARAM;
}

