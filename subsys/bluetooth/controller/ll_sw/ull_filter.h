/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_filter_adv_scan_state_cb(u8_t bm);
void ull_filter_adv_update(u8_t adv_fp);
void ull_filter_scan_update(u8_t scan_fp);
void ull_filter_rpa_update(bool timeout);
void ull_filter_adv_pdu_update(struct ll_adv_set *adv, u8_t idx,
			  struct pdu_adv *pdu);
u8_t ull_filter_rl_find(u8_t id_addr_type, u8_t *id_addr, u8_t *free);
void ull_filter_reset(bool init);
