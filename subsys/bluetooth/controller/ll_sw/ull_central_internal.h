/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ull_central_reset(void);
void ull_central_cleanup(struct node_rx_hdr *rx_free);
void ull_central_setup(struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		      struct lll_conn *lll);
void ull_central_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			  uint32_t remainder, uint16_t lazy, uint8_t force,
			  void *param);
uint8_t ull_central_chm_update(void);
