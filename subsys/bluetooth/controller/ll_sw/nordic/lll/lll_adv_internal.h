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
						      void **extra_data,
						      uint8_t *idx)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	return lll_adv_pdu_and_extra_data_alloc(&lll->data, extra_data, idx);
#else
	return lll_adv_pdu_alloc(&lll->data, idx);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
}

static inline void lll_adv_sync_data_release(struct lll_adv_sync *lll)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	lll_adv_and_extra_data_release(&lll->data);
#else
	lll_adv_data_release(&lll->data);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
}

static inline void lll_adv_sync_data_enqueue(struct lll_adv_sync *lll,
					     uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->data, idx);
}

static inline struct pdu_adv *lll_adv_sync_data_peek(struct lll_adv_sync *lll,
						     void **extra_data)
{
	uint8_t last = lll->data.last;

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	if (extra_data) {
		*extra_data = lll->data.extra_data[last];
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

	return (void *)lll->data.pdu[last];
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

struct pdu_adv *lll_adv_pdu_latest_get(struct lll_adv_pdu *pdu,
				       uint8_t *is_modified);

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
struct pdu_adv *lll_adv_pdu_and_extra_data_latest_get(struct lll_adv_pdu *pdu,
						      void **extra_data,
						      uint8_t *is_modified);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

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
lll_adv_sync_data_latest_get(struct lll_adv_sync *lll, void **extra_data,
			     uint8_t *is_modified)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	return lll_adv_pdu_and_extra_data_latest_get(&lll->data, extra_data,
						     is_modified);
#else
	return lll_adv_pdu_latest_get(&lll->data, is_modified);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
}

static inline struct pdu_adv *
lll_adv_sync_data_curr_get(struct lll_adv_sync *lll, void **extra_data)
{
	uint8_t first = lll->data.first;
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	if (extra_data) {
		*extra_data = lll->data.extra_data[first];
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
	return (void *)lll->data.pdu[first];
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
