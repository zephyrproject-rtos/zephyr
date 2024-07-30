/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void ull_filter_reset(bool init);
bool ull_filter_ull_pal_addr_match(const uint8_t addr_type,
				   const uint8_t *const addr);
bool ull_filter_ull_pal_match(const uint8_t addr_type,
			      const uint8_t *const addr, const uint8_t sid);
bool ull_filter_ull_pal_listed(const uint8_t rl_idx, uint8_t *const addr_type,
			       uint8_t *const addr);

void ull_filter_adv_scan_state_cb(uint8_t bm);
void ull_filter_adv_update(uint8_t adv_fp);
void ull_filter_scan_update(uint8_t scan_fp);
void ull_filter_rpa_update(bool timeout);
const uint8_t *ull_filter_adva_get(uint8_t rl_idx);
const uint8_t *ull_filter_tgta_get(uint8_t rl_idx);
uint8_t ull_filter_rl_find(uint8_t id_addr_type, uint8_t const *const id_addr,
			uint8_t *const free);
