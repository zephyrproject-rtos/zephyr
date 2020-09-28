/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct pdu_adv *lll_adv_pdu_latest_get(struct lll_adv_pdu *pdu,
				       uint8_t *is_modified);

static inline struct pdu_adv *lll_adv_data_latest_get(struct lll_adv *lll,
						      uint8_t *is_modified)
{
	return lll_adv_pdu_latest_get(&lll->adv_data, is_modified);
}

static inline struct pdu_adv *lll_adv_scan_rsp_latest_get(struct lll_adv *lll,
							  uint8_t *is_modified)
{
	return lll_adv_pdu_latest_get(&lll->scan_rsp, is_modified);
}

static inline struct pdu_adv *lll_adv_data_curr_get(struct lll_adv *lll)
{
	return (void *)lll->adv_data.pdu[lll->adv_data.first];
}

static inline struct pdu_adv *lll_adv_scan_rsp_curr_get(struct lll_adv *lll)
{
	return (void *)lll->scan_rsp.pdu[lll->scan_rsp.first];
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static inline struct pdu_adv *
lll_adv_aux_data_latest_get(struct lll_adv_aux *lll, uint8_t *is_modified)
{
	return lll_adv_pdu_latest_get(&lll->data, is_modified);
}

static inline struct pdu_adv *lll_adv_aux_data_curr_get(struct lll_adv_aux *lll)
{
	return (void *)lll->data.pdu[lll->data.first];
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static inline struct pdu_adv *
lll_adv_sync_data_latest_get(struct lll_adv_sync *lll, uint8_t *is_modified)
{
	return lll_adv_pdu_latest_get(&lll->data, is_modified);
}

static inline struct pdu_adv *
lll_adv_sync_data_curr_get(struct lll_adv_sync *lll)
{
	return (void *)lll->data.pdu[lll->data.first];
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

bool lll_adv_scan_req_check(struct lll_adv *lll, struct pdu_adv *sr,
			    uint8_t tx_addr, uint8_t *addr,
			    uint8_t devmatch_ok, uint8_t *rl_idx);

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
int lll_adv_scan_req_report(struct lll_adv *lll, struct pdu_adv *pdu_adv_rx,
			    uint8_t rl_idx, uint8_t rssi_ready);
#endif

bool lll_adv_connect_ind_check(struct lll_adv *lll, struct pdu_adv *ci,
			       uint8_t tx_addr, uint8_t *addr,
			       uint8_t rx_addr, uint8_t *tgt_addr,
			       uint8_t devmatch_ok, uint8_t *rl_idx);
