/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE            8
#define FILTER_IDX_NONE    0xFF

struct ll_filter {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

struct ll_filter *ctrl_filter_get(bool whitelist);
void ll_adv_scan_state_cb(u8_t bm);
