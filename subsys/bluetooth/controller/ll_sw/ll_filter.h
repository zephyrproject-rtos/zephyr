/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE        8

struct ll_wl {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
	u8_t  anon;
};

struct ll_wl *ctrl_wl_get(void);

