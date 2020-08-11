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

	uint8_t sid;
	uint8_t adv_addr_type:1;
	uint8_t filter_policy:1;
	uint8_t state:2;

	uint8_t adv_addr[BDADDR_SIZE];
	uint16_t skip;
	uint16_t skip_countdown;
	uint16_t timeout;

	/* TODO: handling of sync CTE type */
};
