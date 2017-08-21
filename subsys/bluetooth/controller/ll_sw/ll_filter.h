/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WL_SIZE        8

#define FILTER_IDX_NONE    0xFF

struct ll_filter {
	u8_t  enable_bitmask;
	u8_t  addr_type_bitmask;
	u8_t  bdaddr[WL_SIZE][BDADDR_SIZE];
};

void ll_filter_reset(bool init);
void ll_filters_adv_update(u8_t adv_fp);
void ll_filters_scan_update(u8_t scan_fp);

struct ll_filter *ctrl_filter_get(bool whitelist);
bt_addr_t *ctrl_lrpa_get(u8_t rl_idx);
u8_t *ctrl_irks_get(u8_t *count);
u8_t ctrl_rl_idx(bool whitelist, u8_t devmatch_id);
u8_t ctrl_rl_irk_idx(u8_t irkmatch_id);
bool ctrl_irk_whitelisted(u8_t rl_idx);

bool ctrl_rl_enabled(void);
void ll_rl_rpa_update(bool timeout);

u8_t ll_rl_find(u8_t id_addr_type, u8_t *id_addr, u8_t *free);
bool ctrl_rl_addr_allowed(u8_t id_addr_type, u8_t *id_addr, u8_t *rl_idx);
bool ctrl_rl_addr_resolve(u8_t id_addr_type, u8_t *id_addr, u8_t rl_idx);
bool ctrl_rl_idx_allowed(u8_t irkmatch_ok, u8_t rl_idx);
void ll_rl_pdu_adv_update(u8_t idx, struct pdu_adv *pdu);
