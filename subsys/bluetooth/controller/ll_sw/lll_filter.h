/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE            8
#define FILTER_IDX_NONE    0xFF

struct lll_filter {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

extern bt_addr_t *ull_filter_lll_lrpa_get(u8_t rl_idx);
extern u8_t *ull_filter_lll_irks_get(u8_t *count);
extern u8_t ull_filter_lll_rl_idx(bool whitelist, u8_t devmatch_id);
extern u8_t ull_filter_lll_rl_irk_idx(u8_t irkmatch_id);
extern bool ull_filter_lll_irk_whitelisted(u8_t rl_idx);
extern struct lll_filter *ull_filter_lll_get(bool whitelist);
extern bool ull_filter_lll_rl_idx_allowed(u8_t irkmatch_ok, u8_t rl_idx);
extern bool ull_filter_lll_rl_addr_allowed(u8_t id_addr_type, u8_t *id_addr,
					   u8_t *rl_idx);
extern bool ull_filter_lll_rl_addr_resolve(u8_t id_addr_type, u8_t *id_addr,
					   u8_t rl_idx);
extern bool ull_filter_lll_rl_enabled(void);
