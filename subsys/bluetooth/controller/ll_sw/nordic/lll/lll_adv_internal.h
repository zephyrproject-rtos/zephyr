/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline struct pdu_adv *lll_adv_pdu_latest_get(struct lll_adv_pdu *pdu,
						     uint8_t *is_modified)
{
	uint8_t first;

	first = pdu->first;
	if (first != pdu->last) {
		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		pdu->first = first;
		*is_modified = 1U;
	}

	return (void *)pdu->pdu[first];
}

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
