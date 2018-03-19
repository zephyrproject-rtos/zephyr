/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_scan_set {
	struct ull_hdr  ull;
	struct lll_scan lll;
	struct evt_hdr  evt;

	u8_t is_enabled:1;
	u8_t own_addr_type:2;

	u16_t interval;
};
