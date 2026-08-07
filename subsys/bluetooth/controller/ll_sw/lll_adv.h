/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct lll_adv_iso_stream {
	/* Associated BIG Handle */
	uint8_t big_handle;
	struct ll_iso_datapath *dp;

	/* Transmission queue */
	MEMQ_DECLARE(tx);
	memq_link_t link_tx;
	memq_link_t *link_tx_free;

	/* Downstream last packet sequence number */
	uint16_t pkt_seq_num;
};

struct lll_adv_iso_data_chan {
	uint16_t prn_s;
	uint16_t remap_idx;
};

struct lll_adv_iso_data_chan_interleaved {
	uint16_t prn_s;
	uint16_t remap_idx;
	uint16_t id;
};

struct lll_adv_iso {
	struct lll_hdr hdr;
	struct lll_adv *adv;

	uint8_t seed_access_addr[4];
	uint8_t base_crc_init[2];
	uint16_t latency_prepare;
	uint16_t latency_event;
	union {
		struct lll_adv_iso_data_chan data_chan;

#if defined(CONFIG_BT_CTLR_ADV_ISO_INTERLEAVED)
		struct lll_adv_iso_data_chan_interleaved
			interleaved_data_chan[BT_CTLR_ADV_ISO_STREAM_MAX];
#endif /* CONFIG_BT_CTLR_ADV_ISO_INTERLEAVED */
	};
	uint8_t  next_chan_use;

	uint64_t payload_count:39;
	uint64_t enc:1;
	uint64_t framing:1;
	uint64_t handle:8;
	uint64_t cssn:3;
	uint32_t iso_interval:12;

	uint8_t data_chan_map[PDU_CHANNEL_MAP_SIZE];
	uint8_t data_chan_count:6;
	uint8_t num_bis:5;
	uint8_t bn:3;
	uint8_t nse:5;
	uint8_t phy:3;

	uint32_t sub_interval:20;
	uint32_t max_pdu:8;
	uint32_t pto:4;

	uint32_t bis_spacing:20;
	uint32_t max_sdu:8;
	uint32_t irc:4;

	uint32_t sdu_interval:20;
	uint32_t irc_curr:4;
	uint32_t ptc_curr:4;
	uint32_t ptc:4;

	uint8_t bn_curr:3;
	uint8_t bis_curr:5;

	uint8_t phy_flags:1;

	#define CHM_STATE_MASK BIT_MASK(2U)
	#define CHM_STATE_REQ  BIT(0U)
	#define CHM_STATE_SEND BIT(1U)
	uint8_t volatile chm_ack;
	uint8_t          chm_req;
	uint8_t chm_chan_map[PDU_CHANNEL_MAP_SIZE];
	uint8_t chm_chan_count:6;

	uint8_t term_req:1;
	uint8_t term_ack:1;
	uint8_t term_reason;

	uint8_t  ctrl_expire;
	uint16_t ctrl_instant;

	/* Encryption */
	uint8_t giv[8];
	struct ccm ccm_tx;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	/* contains the offset in ticks from the adv_sync pointing to this ISO */
	uint32_t ticks_sync_pdu_offset;
	uint16_t iso_lazy;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	uint16_t stream_handle[BT_CTLR_ADV_ISO_STREAM_MAX];

#if defined(HAL_RADIO_GPIO_HAVE_PA_PIN)
	uint16_t pa_iss_us;
#endif /* HAL_RADIO_GPIO_HAVE_PA_PIN */
};

struct lll_adv_sync {
	struct lll_hdr hdr;
	struct lll_adv *adv;

	uint8_t access_addr[4];
	uint8_t crc_init[3];

	uint16_t latency_prepare;
	uint16_t latency_event;
	uint16_t event_counter;

	uint16_t data_chan_id;
	struct {
		uint8_t data_chan_map[PDU_CHANNEL_MAP_SIZE];
		uint8_t data_chan_count:6;
	} chm[DOUBLE_BUFFER_SIZE];
	uint8_t  chm_first;
	uint8_t  chm_last;
	uint16_t chm_instant;

	struct lll_adv_pdu data;

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	/* Implementation defined radio event counter to calculate chain
	 * PDU channel index.
	 */
	uint16_t data_chan_counter;

	struct pdu_adv *last_pdu;
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	/* contains the offset in us from adv_aux pointing to this sync */
	uint32_t us_adv_sync_pdu_offset;
	uint16_t sync_lazy;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	struct lll_adv_iso *iso;
	uint8_t    volatile iso_chm_done_req;
	uint8_t             iso_chm_done_ack;
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* This flag is used only by LLL. It holds information if CTE
	 * transmission was started by LLL.
	 */
	uint8_t cte_started:1;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
};

struct lll_adv_aux {
	struct lll_hdr hdr;
	struct lll_adv *adv;

	/* Implementation defined radio event counter to calculate auxiliary
	 * PDU channel index.
	 */
	uint16_t data_chan_counter;

	/* Store used by primary channel PDU event to fill the
	 * auxiliary offset to this auxiliary PDU event.
	 */
	uint32_t ticks_pri_pdu_offset;
	uint32_t us_pri_pdu_offset;

	struct lll_adv_pdu data;
#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	struct pdu_adv     *last_pdu;
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */
};

struct lll_adv {
	struct lll_hdr hdr;

#if defined(CONFIG_BT_PERIPHERAL)
	/* NOTE: conn context SHALL be after lll_hdr,
	 *       check ull_conn_setup how it access the connection LLL
	 *       context.
	 */
	struct lll_conn *conn;
	uint8_t is_hdcd:1;
#endif /* CONFIG_BT_PERIPHERAL */

	uint8_t chan_map:3;
	uint8_t chan_map_curr:3;
	uint8_t filter_policy:2;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	uint8_t phy_p:3;
	uint8_t phy_s:3;
	uint8_t phy_flags:1;
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
	struct node_rx_pdu *node_rx_adv_term;
	struct lll_adv_aux *aux;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	struct lll_adv_sync *sync;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	int8_t tx_pwr_lvl;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
};

int lll_adv_init(void);
int lll_adv_reset(void);
void lll_adv_prepare(void *param);

extern uint16_t ull_adv_lll_handle_get(struct lll_adv *lll);
