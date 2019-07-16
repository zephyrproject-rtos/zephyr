/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_conn {
	struct evt_hdr  evt;
	struct ull_hdr  ull;
	struct lll_conn lll;

	u16_t connect_expire;
	u16_t supervision_reload;
	u16_t supervision_expire;
	u16_t procedure_reload;
	u16_t procedure_expire;

#if defined(CONFIG_BT_CTLR_LE_PING)
	u16_t appto_reload;
	u16_t appto_expire;
	u16_t apto_reload;
	u16_t apto_expire;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	u16_t default_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

	union {
		struct {
			u8_t fex_valid:1;
		} common;

		struct {
			u8_t fex_valid:1;
			u32_t force;
			u32_t ticks_to_offset;
		} slave;

		struct {
			u8_t fex_valid:1;
		} master;
	};

	u8_t llcp_req;
	u8_t llcp_ack;
	u8_t llcp_type;

	union {
		struct {
			enum {
				LLCP_CUI_STATE_INPROG,
				LLCP_CUI_STATE_USE,
				LLCP_CUI_STATE_SELECT
			} state:2 __packed;
			u8_t  is_internal:1;
			u16_t interval;
			u16_t latency;
			u16_t timeout;
			u16_t instant;
			u32_t win_offset_us;
			u8_t  win_size;
			u16_t *pdu_win_offset;
			u32_t ticks_anchor;
		} conn_upd;

		struct {
			u8_t  initiate;
			u8_t  chm[5];
			u16_t instant;
		} chan_map;

#if defined(CONFIG_BT_CTLR_PHY)
		struct {
			u8_t initiate:1;
			u8_t cmd:1;
			u8_t tx:3;
			u8_t rx:3;
			u16_t instant;
		} phy_upd_ind;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LE_ENC)
		struct {
			u8_t  initiate;
			u8_t  error_code;
			u8_t  skd[16];
		} encryption;
#endif /* CONFIG_BT_CTLR_LE_ENC */
	} llcp;

	struct node_rx_pdu *llcp_rx;

	u32_t llcp_features;

	struct {
		u8_t  tx:1;
		u8_t  rx:1;
		u8_t  version_number;
		u16_t company_id;
		u16_t sub_version_number;
	} llcp_version;

	struct {
		u8_t req;
		u8_t ack;
		u8_t reason_own;
		u8_t reason_peer;
		struct {
			struct node_rx_hdr hdr;
			u8_t reason;
		} node_rx;
	} llcp_terminate;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	struct {
		u8_t req;
		u8_t ack;
		u8_t pause_rx:1;
		u8_t pause_tx:1;
		u8_t refresh:1;
		u8_t ediv[2];
		u8_t rand[8];
		u8_t ltk[16];
	} llcp_enc;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	struct {
		u8_t  req;
		u8_t  ack;
		enum {
			LLCP_CPR_STATE_REQ,
			LLCP_CPR_STATE_RSP,
			LLCP_CPR_STATE_APP_REQ,
			LLCP_CPR_STATE_APP_WAIT,
			LLCP_CPR_STATE_RSP_WAIT,
			LLCP_CPR_STATE_UPD
		} state:3 __packed;
		u8_t  cmd:1;
		u8_t  disabled:1;
		u8_t  status;
		u16_t interval_min;
		u16_t interval_max;
		u16_t latency;
		u16_t timeout;
		u8_t  preferred_periodicity;
		u16_t reference_conn_event_count;
		u16_t offset0;
		u16_t offset1;
		u16_t offset2;
		u16_t offset3;
		u16_t offset4;
		u16_t offset5;
		u16_t *pdu_win_offset0;
		u32_t ticks_ref;
		u32_t ticks_to_offset_next;
	} llcp_conn_param;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	struct {
		u8_t  req;
		u8_t  ack;
		u8_t  state:2;
#define LLCP_LENGTH_STATE_REQ        0
#define LLCP_LENGTH_STATE_ACK_WAIT   1
#define LLCP_LENGTH_STATE_RSP_WAIT   2
#define LLCP_LENGTH_STATE_RESIZE     3
		u8_t  pause_tx:1;
		u16_t rx_octets;
		u16_t tx_octets;
#if defined(CONFIG_BT_CTLR_PHY)
		u16_t rx_time;
		u16_t tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
	} llcp_length;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	struct {
		u8_t req;
		u8_t ack;
		u8_t state:2;
#define LLCP_PHY_STATE_REQ      0
#define LLCP_PHY_STATE_ACK_WAIT 1
#define LLCP_PHY_STATE_RSP_WAIT 2
#define LLCP_PHY_STATE_UPD      3
		u8_t tx:3;
		u8_t rx:3;
		u8_t pause_tx:1;
		u8_t flags:1;
		u8_t cmd:1;
	} llcp_phy;

	u8_t phy_pref_tx:3;
	u8_t phy_pref_flags:1;
	u8_t phy_pref_rx:3;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
	/* Detect empty L2CAP start frame */
	u8_t  start_empty:1;
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */

	struct node_tx *tx_head;
	struct node_tx *tx_ctrl;
	struct node_tx *tx_ctrl_last;
	struct node_tx *tx_data;
	struct node_tx *tx_data_last;

	u8_t chm_updated;
};

struct node_rx_cc {
	u8_t  status;
	u8_t  role;
	u8_t  peer_addr_type;
	u8_t  peer_addr[BDADDR_SIZE];
#if defined(CONFIG_BT_CTLR_PRIVACY)
	u8_t  peer_rpa[BDADDR_SIZE];
	u8_t  own_addr_type;
	u8_t  own_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_PRIVACY */
	u16_t interval;
	u16_t latency;
	u16_t timeout;
	u8_t  sca;
};

struct node_rx_cu {
	u8_t  status;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
};

struct node_rx_cs {
	u8_t csa;
};

struct node_rx_pu {
	u8_t status;
	u8_t tx;
	u8_t rx;
};
