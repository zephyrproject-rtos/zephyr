/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_filter_adv_scan_state_cb(uint8_t bm);
void ull_filter_adv_update(uint8_t adv_fp);
void ull_filter_scan_update(uint8_t scan_fp);
void ull_filter_rpa_update(bool timeout);
uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			uint8_t *const free);
void ull_filter_reset(bool init);
const uint8_t *ull_filter_adva_get(struct ll_adv_set *adv);
const uint8_t *ull_filter_tgta_get(struct ll_adv_set *adv);
