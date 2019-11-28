/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "lll.h"
#include "ctrl.h"
#include "ll.h"
#include "ll_filter.h"

static struct {
	u16_t interval;
	u16_t window;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u8_t  type:4;
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	u8_t  type:1;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	u8_t  own_addr_type:2;
	u8_t  filter_policy:2;
} ll_scan;

u8_t ll_scan_params_set(u8_t type, u16_t interval, u16_t window,
			 u8_t own_addr_type, u8_t filter_policy)
{
	if (ll_scan_is_enabled(0)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* type value:
	 * 0000b - legacy 1M passive
	 * 0001b - legacy 1M active
	 * 0010b - Ext. 1M passive
	 * 0011b - Ext. 1M active
	 * 0100b - invalid
	 * 0101b - invalid
	 * 0110b - invalid
	 * 0111b - invalid
	 * 1000b - Ext. Coded passive
	 * 1001b - Ext. Coded active
	 */
	ll_scan.type = type;
	ll_scan.interval = interval;
	ll_scan.window = window;
	ll_scan.own_addr_type = own_addr_type;
	ll_scan.filter_policy = filter_policy;

	return 0;
}

u8_t ll_scan_enable(u8_t enable)
{
	u8_t rpa_gen = 0U;
	u32_t status;
	u32_t scan;

	if (!enable) {
		return radio_scan_disable(true);
	}

	scan = ll_scan_is_enabled(0);

	/* Initiator and scanning are not supported */
	if (scan & BIT(2)) {
	       return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (scan) {
		/* Duplicate filtering is processed in the HCI layer */
		return 0;
	}

	if (ll_scan.own_addr_type & 0x1) {
		if (!mem_nz(ll_addr_get(1, NULL), BDADDR_SIZE)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	ll_filters_scan_update(ll_scan.filter_policy);

	if ((ll_scan.type & 0x1) &&
	    (ll_scan.own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	     ll_scan.own_addr_type == BT_ADDR_LE_RANDOM_ID)) {
		/* Generate RPAs if required */
		ll_rl_rpa_update(false);
		rpa_gen = 1U;
	}
#endif
	status = radio_scan_enable(ll_scan.type, ll_scan.own_addr_type & 0x1,
				   ll_addr_get(ll_scan.own_addr_type & 0x1,
					       NULL),
				   ll_scan.interval, ll_scan.window,
				   ll_scan.filter_policy, rpa_gen,
				   FILTER_IDX_NONE);

	return status;
}
