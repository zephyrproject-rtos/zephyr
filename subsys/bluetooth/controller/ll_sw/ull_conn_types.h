/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define IS_ACL_HANDLE(_handle) ((_handle) < CONFIG_BT_MAX_CONN)

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

/*
 * to reduce length and unreadability of the ll_conn struct the
 * structures inside it have been defined first
 */
struct llcp_struct {
	/* Local Request */
	struct {
		sys_slist_t pend_proc_list;
		uint8_t state;
		/* Procedure Response Timeout timer expire value */
		uint16_t prt_expire;
		uint8_t pause;
	} local;

	/* Remote Request */
	struct {
		sys_slist_t pend_proc_list;
		uint8_t state;
		/* Procedure Response Timeout timer expire value */
		uint16_t prt_expire;
		uint8_t pause;
		uint8_t collision;
		uint8_t incompat;
		uint8_t reject_opcode;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
		uint8_t paused_cmd;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP || CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	} remote;

	/* Procedure Response Timeout timer reload value */
	uint16_t prt_reload;

	/* Prepare parameters */
	struct {
		uint32_t ticks_at_expire; /* Vendor specific tick units */
		uint32_t remainder;       /* Vendor specific remainder fraction of a tick unit */
		uint16_t lazy;            /* Previous skipped radio event count */
	} prep;

	/* Version Exchange Procedure State */
	struct {
		uint8_t sent;
		uint8_t valid;
		struct pdu_data_llctrl_version_ind cached;
	} vex;

	/*
	 * As of today only 36 feature bits are in use,
	 * so some optimisation is possible
	 * we also need to keep track of the features of the
	 * other node, so that we can send a proper
	 * reply over HCI to the host
	 * see BT Core spec 5.2 Vol 6, Part B, sec. 5.1.4
	 */
	struct {
		uint8_t valid;
		/*
		 * Stores features supported by peer device. The content of the member may be
		 * verified when feature exchange procedure has completed, valid member is set to 1.
		 */
		uint64_t features_peer;
		/*
		 * Stores features common for two connected devices. Before feature exchange
		 * procedure is completed, the member stores information about all features
		 * supported by local device. After completion of the procedure, the feature set
		 * may be limited to features that are common.
		 */
		uint64_t features_used;
	} fex;

	/* Minimum used channels procedure state */
	struct {
		uint8_t phys;
		uint8_t min_used_chans;
	} muc;

	/* TODO: we'll need the next few structs eventually,
	 * Thomas and Szymon please comment on names etc.
	 */
	struct {
		uint16_t *pdu_win_offset;
		uint32_t ticks_anchor;
	} conn_upd;

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	/* @brief Constant Tone Extension configuration for CTE request control procedure. */
	struct llcp_df_req_cfg {
		/* Procedure may be active periodically, active state must be stored.
		 * If procedure is active, request parameters update may not be issued.
		 */
		volatile uint8_t is_enabled;
		uint8_t cte_type;
		/* Minimum requested CTE length in 8us units */
		uint8_t min_cte_len;
		uint16_t req_interval;
		uint16_t req_expire;
	} cte_req;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	struct llcp_df_rsp_cfg {
		uint8_t is_enabled:1;
		uint8_t is_active:1;
		uint8_t cte_types;
		uint8_t max_cte_len;
		void *disable_param;
		void (*disable_cb)(void *param);
	} cte_rsp;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

	struct {
		uint8_t terminate_ack;
	} cis;

	uint8_t tx_buffer_alloc;
	uint8_t tx_q_pause_data_mask;
}; /* struct llcp_struct */

struct ll_conn {
	struct ull_hdr  ull;
	struct lll_conn lll;

	struct ull_tx_q tx_q;
	struct llcp_struct llcp;

	struct {
		uint8_t reason_final;
		/* node rx type with memory aligned storage for terminate
		 * reason.
		 * HCI will reference the value using the pdu member of
		 * struct node_rx_pdu.
		 */
		struct {
			struct node_rx_hdr hdr;

			uint8_t reason __aligned(4);
		} node_rx;
	} llcp_terminate;

/*
 * TODO: all the following comes from the legacy LL llcp structure
 * and/or needs to be properly integrated in the control procedures
 */
	union {
		struct {
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t  is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
		} common;
#if defined(CONFIG_BT_PERIPHERAL)
		struct {
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t  is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
			uint8_t  latency_cancel:1;
			uint8_t  sca:3;
			uint32_t force;
			uint32_t ticks_to_offset;
		} periph;
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		struct {
#if defined(CONFIG_BT_CTLR_CONN_META)
			uint8_t  is_must_expire:1;
#endif /* CONFIG_BT_CTLR_CONN_META */
			uint8_t terminate_ack:1;
		} central;
#endif /* CONFIG_BT_CENTRAL */
	};

	/* Cancel the prepare in the instant a Connection Update takes place */
	uint8_t cancel_prepare:1;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* Pause Rx data PDU's */
	uint8_t pause_rx_data:1;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_LE_PING)
	uint16_t appto_reload;
	uint16_t appto_expire;
	uint16_t apto_reload;
	uint16_t apto_expire;
#endif /* CONFIG_BT_CTLR_LE_PING */

	uint16_t connect_expire;
	uint16_t supervision_timeout;
	uint16_t supervision_expire;
	uint32_t connect_accept_to;

#if defined(CONFIG_BT_CTLR_PHY)
	uint8_t phy_pref_tx:3;
	uint8_t phy_pref_rx:3;
#endif /* CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	uint16_t default_tx_octets;

#if defined(CONFIG_BT_CTLR_PHY)
	uint16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN)
	uint8_t own_id_addr_type:1;
	uint8_t peer_id_addr_type:1;
	uint8_t own_id_addr[BDADDR_SIZE];
	uint8_t peer_id_addr[BDADDR_SIZE];
#endif /* CONFIG_BT_CTLR_CHECK_SAME_PEER_CONN */

#if defined(CONFIG_BT_CTLR_LLID_DATA_START_EMPTY)
	/* Detect empty L2CAP start frame */
	uint8_t  start_empty:1;
#endif /* CONFIG_BT_CTLR_LLID_DATA_START_EMPTY */
}; /* struct ll_conn */

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

struct node_rx_sca {
	uint8_t status;
	uint8_t sca;
};
