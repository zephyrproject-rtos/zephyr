/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void lll_scan_isr_resume(void *param);
bool lll_scan_isr_rx_check(const struct lll_scan *lll, uint8_t irkmatch_ok,
			   uint8_t devmatch_ok, uint8_t rl_idx);
bool lll_scan_adva_check(const struct lll_scan *lll, uint8_t addr_type,
			 const uint8_t *addr, uint8_t rl_idx);
bool lll_scan_ext_tgta_check(const struct lll_scan *lll, bool pri, bool is_init,
			     const struct pdu_adv *pdu, uint8_t rl_idx,
			     bool *const dir_report);
void lll_scan_prepare_connect_req(struct lll_scan *lll, struct pdu_adv *pdu_tx,
				  uint8_t phy, uint8_t adv_tx_addr,
				  uint8_t *adv_addr, uint8_t init_tx_addr,
				  uint8_t *init_addr, uint32_t *conn_space_us);
uint8_t lll_scan_aux_setup(struct pdu_adv *pdu, uint8_t pdu_phy,
			   uint8_t pdu_phy_flags_rx, radio_isr_cb_t setup_cb,
			   void *param);
void lll_scan_aux_isr_aux_setup(void *param);
bool lll_scan_aux_addr_match_get(const struct lll_scan *lll,
				 const struct pdu_adv *pdu,
				 uint8_t *const devmatch_ok,
				 uint8_t *const devmatch_id,
				 uint8_t *const irkmatch_ok,
				 uint8_t *const irkmatch_id);
