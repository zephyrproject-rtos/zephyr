/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void lll_scan_prepare_connect_req(struct lll_scan *lll, struct pdu_adv *pdu_tx,
				  uint8_t adv_tx_addr, uint8_t *adv_addr,
				  uint8_t init_tx_addr, uint8_t *init_addr,
				  uint32_t *conn_space_us);
