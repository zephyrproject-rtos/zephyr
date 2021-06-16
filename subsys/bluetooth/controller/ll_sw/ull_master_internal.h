/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_master_cleanup(struct node_rx_hdr *rx_free);
void ull_master_setup(struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		      struct lll_conn *lll);
void ull_master_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
			  uint16_t lazy, uint8_t force, void *param);
