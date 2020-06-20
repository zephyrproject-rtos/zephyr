/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_filter_adv_scan_state_cb(uint8_t bm);
void ull_filter_adv_update(uint8_t adv_fp);
void ull_filter_scan_update(uint8_t scan_fp);
void ull_filter_rpa_update(bool timeout);
void ull_filter_adv_pdu_update(struct ll_adv_set *adv, struct pdu_adv *pdu);
uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			uint8_t *const free);
void ull_filter_reset(bool init);
