/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <bluetooth/hci.h>

#include "util/util.h"

#include "pdu.h"
#include "ctrl.h"
#include "ll.h"
#include "ll_filter.h"

u32_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			   u8_t filter_policy, u8_t peer_addr_type,
			   u8_t *peer_addr, u8_t own_addr_type,
			   u16_t interval, u16_t latency,
			   u16_t timeout)
{
	u32_t status;
	u8_t  rpa_gen = 0;
	u8_t  rl_idx = FILTER_IDX_NONE;

	if (radio_scan_is_enabled()) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	status = radio_connect_enable(peer_addr_type, peer_addr, interval,
				      latency, timeout);

	if (status) {
		return status;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	ll_filters_scan_update(filter_policy);

	if (!filter_policy && ctrl_rl_enabled()) {
		/* Look up the resolving list */
		rl_idx = ll_rl_find(peer_addr_type, peer_addr, NULL);
	}

	if (own_addr_type == BT_ADDR_LE_PUBLIC_ID ||
	    own_addr_type == BT_ADDR_LE_RANDOM_ID) {

		/* Generate RPAs if required */
		ll_rl_rpa_update(false);
		own_addr_type &= 0x1;
		rpa_gen = 1;
	}
#endif
	return radio_scan_enable(0, own_addr_type,
				 ll_addr_get(own_addr_type, NULL),
				 scan_interval, scan_window,
				 filter_policy, rpa_gen, rl_idx);
}
