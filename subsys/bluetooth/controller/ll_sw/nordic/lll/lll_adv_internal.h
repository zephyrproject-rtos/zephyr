/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

bool lll_adv_scan_req_check(struct lll_adv *lll, struct pdu_adv *sr,
			    uint8_t tx_addr, uint8_t *addr,
			    uint8_t rx_addr, uint8_t *tgt_addr,
			    uint8_t devmatch_ok, uint8_t *rl_idx);

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
int lll_adv_scan_req_report(struct lll_adv *lll, struct pdu_adv *pdu_adv_rx,
			    uint8_t rl_idx, uint8_t rssi_ready);
#endif

bool lll_adv_connect_ind_check(struct lll_adv *lll, struct pdu_adv *ci,
			       uint8_t tx_addr, uint8_t *addr,
			       uint8_t rx_addr, uint8_t *tgt_addr,
			       uint8_t devmatch_ok, uint8_t *rl_idx);
