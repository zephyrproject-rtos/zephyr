/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_ADV_SET)
#define BT_CTLR_ADV_SET CONFIG_BT_CTLR_ADV_SET
#else /* CONFIG_BT_CTLR_ADV_SET */
#define BT_CTLR_ADV_SET 1
#endif /* CONFIG_BT_CTLR_ADV_SET */

/* Structure used to double buffer pointers of AD Data PDU buffer.
 * The first and last members are used to make modification to AD data to be
 * context safe. Thread always appends or updates the buffer pointed to
 * the array element indexed by the member last.
 * LLL in the ISR context, checks, traverses to the valid pointer indexed
 * by the member first, such that the buffer is the latest committed by
 * the thread context.
 */
struct lll_adv_pdu {
	uint8_t volatile first;
	uint8_t          last;
	uint8_t          *pdu[DOUBLE_BUFFER_SIZE];
};

struct lll_adv_aux {
	struct lll_hdr hdr;
	struct lll_adv *adv;

	uint32_t ticks_offset;

	struct lll_adv_pdu data;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	int8_t tx_pwr_lvl;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
};

#if IS_ENABLED(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
struct lll_df_adv_cfg;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

struct lll_adv_iso {
	struct lll_hdr hdr;
};

struct lll_adv_sync {
	struct lll_hdr hdr;
	struct lll_adv *adv;
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	struct lll_adv_iso *adv_iso;
#endif /* CONFIG_BT_CTLR_ADV_ISO */

	uint8_t access_addr[4];
	uint8_t crc_init[3];

	uint16_t latency_prepare;
	uint16_t latency_event;
	uint16_t event_counter;

	uint8_t data_chan_map[5];
	uint8_t data_chan_count:6;
	uint16_t data_chan_id;

	uint32_t ticks_offset;

	struct lll_adv_pdu data;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	int8_t tx_pwr_lvl;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

#if IS_ENABLED(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	struct lll_df_adv_cfg *df_cfg;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
};

struct lll_adv {
	struct lll_hdr hdr;

#if defined(CONFIG_BT_PERIPHERAL)
	/* NOTE: conn context has to be after lll_hdr */
	struct lll_conn *conn;
	uint8_t is_hdcd:1;
#endif /* CONFIG_BT_PERIPHERAL */

	uint8_t chan_map:3;
	uint8_t chan_map_curr:3;
	uint8_t filter_policy:2;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint8_t phy_p:3;
	uint8_t phy_s:3;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	uint8_t scan_req_notify:1;
#endif

#if defined(CONFIG_BT_HCI_MESH_EXT)
	uint8_t is_mesh:1;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	struct lll_adv_pdu adv_data;
	struct lll_adv_pdu scan_rsp;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct lll_adv_aux *aux;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct lll_adv_sync *sync;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	int8_t tx_pwr_lvl;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct node_rx_hdr *node_rx_adv_term;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
};

int lll_adv_init(void);
int lll_adv_reset(void);
int lll_adv_data_init(struct lll_adv_pdu *pdu);
int lll_adv_data_reset(struct lll_adv_pdu *pdu);
int lll_adv_data_release(struct lll_adv_pdu *pdu);
struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx);
void lll_adv_prepare(void *param);

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

extern uint16_t ull_adv_lll_handle_get(struct lll_adv *lll);
