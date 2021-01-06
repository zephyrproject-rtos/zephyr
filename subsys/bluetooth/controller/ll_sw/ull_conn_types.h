/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum llcp {
	LLCP_NONE,
	LLCP_CONN_UPD,
	LLCP_CHAN_MAP,

	/*
	 * LLCP_TERMINATE,
	 * LLCP_FEATURE_EXCHANGE,
	 * LLCP_VERSION_EXCHANGE,
	 */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	LLCP_ENCRYPTION,
#endif /* CONFIG_BT_CTLR_LE_ENC */

	LLCP_CONNECTION_PARAM_REQ,

#if defined(CONFIG_BT_CTLR_LE_PING)
	LLCP_PING,
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_PHY)
	LLCP_PHY_UPD,
#endif /* CONFIG_BT_CTLR_PHY */
};

struct ll_conn {
	struct evt_hdr  evt;
	struct ull_hdr  ull;
	struct lll_conn lll;

	uint16_t connect_expire;
	uint16_t supervision_reload;
	uint16_t supervision_expire;
	uint16_t procedure_reload;
	uint16_t procedure_expire;

#if defined(CONFIG_BT_CTLR_LE_PING)
	uint16_t appto_reload;
	uint16_t appto_expire;
	uint16_t apto_reload;
	uint16_t apto_expire;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	uint16_t default_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	uint16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	union {
		struct {
			uint8_t fex_valid:1;
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
		} common;

#if defined(CONFIG_BT_PERIPHERAL)
		struct {
			uint8_t  fex_valid:1;
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t  is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
			uint8_t  latency_cancel:1;
			uint8_t  sca:3;
			uint32_t force;
			uint32_t ticks_to_offset;
		} slave;
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		struct {
			uint8_t fex_valid:1;
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
			uint8_t terminate_ack:1;
		} master;
#endif /* CONFIG_BT_CENTRAL */
	};

	uint8_t llcp_req;
	uint8_t llcp_ack;
	uint8_t llcp_type;

	union {
		struct {
			uint16_t instant;
			uint16_t *pdu_win_offset;
			uint32_t ticks_anchor;
		} conn_upd;

		struct {
			uint8_t  initiate:1;
			uint8_t  chm[5];
			uint16_t instant;
		} chan_map;

#if defined(CONFIG_BT_CTLR_PHY)
		struct {
			uint8_t initiate:1;
			uint8_t cmd:1;
			uint8_t tx:3;
			uint8_t rx:3;
			uint16_t instant;
		} phy_upd_ind;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
		struct {
			enum {
				LLCP_ENC_STATE_INPROG,
				LLCP_ENC_STATE_INIT,
				LLCP_ENC_STATE_LTK_WAIT,
			} state:2 __packed;
			uint8_t  error_code;
			uint8_t  skd[16];
		} encryption;
#endif /* CONFIG_BT_CTLR_LE_ENC */
	} llcp;

	struct node_rx_pdu *llcp_rx;

	struct {
		uint8_t  req;
		uint8_t  ack;
		enum {
			LLCP_CUI_STATE_INPROG,
			LLCP_CUI_STATE_USE,
			LLCP_CUI_STATE_SELECT
		} state:2 __packed;
		uint8_t  cmd:1;
		uint16_t interval;
		uint16_t latency;
		uint16_t timeout;
		uint32_t win_offset_us;
		uint8_t  win_size;
	} llcp_cu;

	struct {
		uint8_t  req;
		uint8_t  ack;
		uint32_t features_conn;
		uint32_t features_peer;
	} llcp_feature;

	struct {
		uint8_t  req;
		uint8_t  ack;
		uint8_t  tx:1;
		uint8_t  rx:1;
		uint8_t  version_number;
		uint16_t company_id;
		uint16_t sub_version_number;
	} llcp_version;

	struct {
		uint8_t req;
		uint8_t ack;
		uint8_t reason_own;
		uint8_t reason_peer;
		/* node rx type with memory aligned storage for terminate
		 * reason.
		 * HCI will reference the value using the pdu member of
		 * struct node_rx_pdu.
		 */
		struct {
			struct node_rx_hdr hdr;
			union {
				uint8_t    pdu[0] __aligned(4);
				uint8_t    reason;
			};
		} node_rx;
	} llcp_terminate;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	struct {
		uint8_t req;
		uint8_t ack;
		uint8_t pause_rx:1;
		uint8_t pause_tx:1;
		uint8_t refresh:1;
		uint8_t ediv[2];
		uint8_t rand[8];
		uint8_t ltk[16];
	} llcp_enc;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	struct {
		uint8_t  req;
		uint8_t  ack;
		enum {
			LLCP_CPR_STATE_REQ,
			LLCP_CPR_STATE_RSP,
			LLCP_CPR_STATE_APP_REQ,
			LLCP_CPR_STATE_APP_WAIT,
			LLCP_CPR_STATE_RSP_WAIT,
			LLCP_CPR_STATE_UPD
		} state:3 __packed;
		uint8_t  cmd:1;
		uint8_t  disabled:1;
		uint8_t  status;
		uint16_t interval_min;
		uint16_t interval_max;
		uint16_t latency;
		uint16_t timeout;
		uint8_t  preferred_periodicity;
		uint16_t reference_conn_event_count;
		uint16_t offset0;
		uint16_t offset1;
		uint16_t offset2;
		uint16_t offset3;
		uint16_t offset4;
		uint16_t offset5;
		uint16_t *pdu_win_offset0;
		uint32_t ticks_ref;
		uint32_t ticks_to_offset_next;
	} llcp_conn_param;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	struct {
		uint8_t  req;
		uint8_t  ack;
		uint8_t  state:3;
#define LLCP_LENGTH_STATE_REQ                 0
#define LLCP_LENGTH_STATE_REQ_ACK_WAIT        1
#define LLCP_LENGTH_STATE_RSP_WAIT            2
#define LLCP_LENGTH_STATE_RSP_ACK_WAIT        3
#define LLCP_LENGTH_STATE_RESIZE              4
#define LLCP_LENGTH_STATE_RESIZE_RSP          5
#define LLCP_LENGTH_STATE_RESIZE_RSP_ACK_WAIT 6
		uint8_t  disabled:1;
		uint16_t rx_octets;
		uint16_t tx_octets;
#if defined(CONFIG_BT_CTLR_PHY)
		uint16_t rx_time;
		uint16_t tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
		struct {
			uint16_t tx_octets;
#if defined(CONFIG_BT_CTLR_PHY)
			uint16_t tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
		} cache;
	} llcp_length;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	struct {
		uint8_t req;
		uint8_t ack;
		uint8_t state:2;
#define LLCP_PHY_STATE_REQ      0
#define LLCP_PHY_STATE_ACK_WAIT 1
#define LLCP_PHY_STATE_RSP_WAIT 2
#define LLCP_PHY_STATE_UPD      3
		uint8_t tx:3;
		uint8_t rx:3;
		uint8_t pause_tx:1;
		uint8_t flags:1;
		uint8_t cmd:1;
		uint8_t disabled:1;
	} llcp_phy;

	uint8_t phy_pref_tx:3;
	uint8_t phy_pref_rx:3;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
	/* Detect empty L2CAP start frame */
	uint8_t  start_empty:1;
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

	struct node_tx *tx_head;
	struct node_tx *tx_ctrl;
	struct node_tx *tx_ctrl_last;
	struct node_tx *tx_data;
	struct node_tx *tx_data_last;

	uint8_t chm_updated;
};

struct node_rx_cc {
	uint8_t  status;
	uint8_t  role;
	uint8_t  peer_addr_type;
	uint8_t  peer_addr[BDADDR_SIZE];
#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t  peer_rpa[BDADDR_SIZE];
	uint8_t  local_rpa[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint8_t  sca;
};

struct node_rx_cu {
	uint8_t  status;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
};

struct node_rx_cs {
	uint8_t csa;
};

struct node_rx_pu {
	uint8_t status;
	uint8_t tx;
	uint8_t rx;
};
