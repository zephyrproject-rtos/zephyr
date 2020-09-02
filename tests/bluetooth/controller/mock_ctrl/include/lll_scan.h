/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_scan {
	struct ull_cp_conn *conn;
	uint8_t  init_addr[BDADDR_SIZE];
	uint8_t  adv_addr[BDADDR_SIZE];
	uint8_t adv_addr_type:1;
	uint16_t conn_timeout;
	uint8_t conn_ticks_slot;
	uint8_t connect_expire;
	uint8_t supervision_expire;
	uint8_t supervision_reload;
	uint8_t procedure_expire;
	uint8_t procedure_reload;
};
