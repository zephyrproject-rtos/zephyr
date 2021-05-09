/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void lll_scan_prepare_connect_req(struct lll_scan *lll, struct pdu_adv *pdu_tx,
				  uint8_t phy, uint8_t adv_tx_addr,
				  uint8_t *adv_addr, uint8_t init_tx_addr,
				  uint8_t *init_addr, uint32_t *conn_space_us);

bool lll_scan_adva_check(struct lll_scan *lll, uint8_t addr_type, uint8_t *addr,
			 uint8_t rl_idx);

bool lll_scan_tgta_check(struct lll_scan *lll, bool init, uint8_t addr_type,
			 uint8_t *addr, uint8_t rl_idx, bool *dir_report);
