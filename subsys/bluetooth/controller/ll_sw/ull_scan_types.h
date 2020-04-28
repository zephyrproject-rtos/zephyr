/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_scan_set {
	struct evt_hdr  evt;
	struct ull_hdr  ull;
	struct lll_scan lll;

	u8_t is_enabled:1;
	u8_t own_addr_type:2;
};

struct ll_scan_aux_set {
	struct evt_hdr      evt;
	struct ull_hdr      ull;
	struct lll_scan_aux lll;

	memq_link_t        *rx_head;
	memq_link_t        *rx_last;
};
