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

u32_t ll_create_connection(u16_t scan_interval, u16_t scan_window,
			   u8_t filter_policy, u8_t peer_addr_type,
			   u8_t *peer_addr, u8_t own_addr_type,
			   u16_t interval, u16_t latency,
			   u16_t timeout)
{
	u32_t status;

	status = radio_connect_enable(peer_addr_type, peer_addr, interval,
				      latency, timeout);

	if (status) {
		return status;
	}

	return radio_scan_enable(0, own_addr_type,
				 ll_addr_get(own_addr_type, NULL),
				 scan_interval, scan_window, filter_policy);
}
