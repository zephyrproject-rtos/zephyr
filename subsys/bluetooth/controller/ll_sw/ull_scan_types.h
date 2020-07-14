/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_scan_set {
	struct evt_hdr  evt;
	struct ull_hdr  ull;
	struct lll_scan lll;

	uint8_t is_enabled:1;
	uint8_t own_addr_type:2;
};

struct ll_scan_aux_set {
	struct evt_hdr      evt;
	struct ull_hdr      ull;
	struct lll_scan_aux lll;

	struct node_rx_hdr *rx_head;
	struct node_rx_hdr *rx_last;
};
