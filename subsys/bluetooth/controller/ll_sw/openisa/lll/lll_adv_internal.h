/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_data_init(struct lll_adv_pdu *pdu);
int lll_adv_data_reset(struct lll_adv_pdu *pdu);
int lll_adv_data_release(struct lll_adv_pdu *pdu);

struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx);

static inline void lll_adv_pdu_enqueue(struct lll_adv_pdu *pdu, uint8_t idx)
{
	pdu->last = idx;
}

static inline struct pdu_adv *lll_adv_data_alloc(struct lll_adv *lll, uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->adv_data, idx);
}

static inline void lll_adv_data_enqueue(struct lll_adv *lll, uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->adv_data, idx);
}

static inline struct pdu_adv *lll_adv_data_peek(struct lll_adv *lll)
{
	return (void *)lll->adv_data.pdu[lll->adv_data.last];
}

static inline struct pdu_adv *lll_adv_scan_rsp_alloc(struct lll_adv *lll,
						     uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->scan_rsp, idx);
}

static inline void lll_adv_scan_rsp_enqueue(struct lll_adv *lll, uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->scan_rsp, idx);
}

static inline struct pdu_adv *lll_adv_scan_rsp_peek(struct lll_adv *lll)
{
	return (void *)lll->scan_rsp.pdu[lll->scan_rsp.last];
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static inline struct pdu_adv *lll_adv_aux_data_alloc(struct lll_adv_aux *lll,
						     uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->data, idx);
}

static inline void lll_adv_aux_data_enqueue(struct lll_adv_aux *lll,
					    uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->data, idx);
}

static inline struct pdu_adv *lll_adv_aux_data_peek(struct lll_adv_aux *lll)
{
	return (void *)lll->data.pdu[lll->data.last];
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static inline struct pdu_adv *lll_adv_sync_data_alloc(struct lll_adv_sync *lll,
						     uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->data, idx);
}

static inline void lll_adv_sync_data_enqueue(struct lll_adv_sync *lll,
					     uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->data, idx);
}

static inline struct pdu_adv *lll_adv_sync_data_peek(struct lll_adv_sync *lll)
{
	return (void *)lll->data.pdu[lll->data.last];
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

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
