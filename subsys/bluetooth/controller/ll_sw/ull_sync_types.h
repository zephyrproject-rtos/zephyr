/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LL_SYNC_STATE_IDLE       0x00
#define LL_SYNC_STATE_ADDR_MATCH 0x01
#define LL_SYNC_STATE_CREATED    0x02

struct ll_sync_set {
	struct evt_hdr evt;
	struct ull_hdr ull;
	struct lll_sync lll;
};

struct node_rx_sync {
	uint8_t status;
	uint8_t  phy;
	uint16_t interval;
	uint8_t  sca;
};
