/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_adv_data_init(struct lll_adv_pdu *pdu);
int lll_adv_data_reset(struct lll_adv_pdu *pdu);
int lll_adv_data_release(struct lll_adv_pdu *pdu);

static inline void lll_adv_pdu_enqueue(struct lll_adv_pdu *pdu, uint8_t idx)
{
	pdu->last = idx;
}

struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx);

static inline struct pdu_adv *lll_adv_data_alloc(struct lll_adv *lll,
						 uint8_t *idx)
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
