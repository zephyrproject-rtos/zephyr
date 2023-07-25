/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_periph_setup(struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		     struct lll_conn *lll);
void ull_periph_latency_cancel(struct ll_conn *conn, uint16_t handle);
void ull_periph_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			 uint32_t remainder, uint16_t lazy, uint8_t force,
			 void *param);
