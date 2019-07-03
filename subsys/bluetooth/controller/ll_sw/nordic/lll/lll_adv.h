/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_adv_pdu {
	u8_t           first;
	u8_t           last;
	/* TODO: use,
	 * struct pdu_adv *pdu[DOUBLE_BUFFER_SIZE];
	 */
	u8_t pdu[DOUBLE_BUFFER_SIZE][PDU_AC_SIZE_MAX];
};

struct lll_adv {
	struct lll_hdr hdr;

#if defined(CONFIG_BT_PERIPHERAL)
	/* NOTE: conn context has to be after lll_hdr */
	struct lll_conn *conn;
	u8_t is_hdcd:1;
#endif /* CONFIG_BT_PERIPHERAL */

	u8_t chan_map:3;
	u8_t chan_map_curr:3;
	u8_t filter_policy:2;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	u8_t phy_p:3;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	u8_t is_mesh:1;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	struct lll_adv_pdu adv_data;
	struct lll_adv_pdu scan_rsp;
};

int lll_adv_init(void);
int lll_adv_reset(void);

void lll_adv_prepare(void *param);

static inline struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu,
						u8_t *idx)
{
	u8_t last;

	if (pdu->first == pdu->last) {
		last = pdu->last + 1;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		last = pdu->last;
	}

	*idx = last;

	return (void *)pdu->pdu[last];
}

static inline void lll_adv_pdu_enqueue(struct lll_adv_pdu *pdu, u8_t idx)
{
	pdu->last = idx;
}

static inline struct pdu_adv *lll_adv_data_alloc(struct lll_adv *lll, u8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->adv_data, idx);
}

static inline void lll_adv_data_enqueue(struct lll_adv *lll, u8_t idx)
{
	lll_adv_pdu_enqueue(&lll->adv_data, idx);
}

static inline struct pdu_adv *lll_adv_data_peek(struct lll_adv *lll)
{
	return (void *)lll->adv_data.pdu[lll->adv_data.last];
}

static inline struct pdu_adv *lll_adv_scan_rsp_alloc(struct lll_adv *lll,
						     u8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->scan_rsp, idx);
}

static inline void lll_adv_scan_rsp_enqueue(struct lll_adv *lll, u8_t idx)
{
	lll_adv_pdu_enqueue(&lll->scan_rsp, idx);
}

static inline struct pdu_adv *lll_adv_scan_rsp_peek(struct lll_adv *lll)
{
	return (void *)lll->scan_rsp.pdu[lll->scan_rsp.last];
}

extern u16_t ull_adv_lll_handle_get(struct lll_adv *lll);
