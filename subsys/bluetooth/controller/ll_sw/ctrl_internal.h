/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum llcp {
	LLCP_NONE,
	LLCP_CONNECTION_UPDATE,
	LLCP_CHAN_MAP,

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_ENC)
	LLCP_ENCRYPTION,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_LE_ENC */

	LLCP_FEATURE_EXCHANGE,
	LLCP_VERSION_EXCHANGE,
	/* LLCP_TERMINATE, */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
	LLCP_PING,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
	LLCP_PHY_UPD,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */
};


struct shdr {
	u32_t ticks_xtal_to_start;
	u32_t ticks_active_to_start;
	u32_t ticks_preempt_to_start;
	u32_t ticks_slot;
};

struct connection {
	struct shdr hdr;

	u8_t  access_addr[4];
	u8_t  crc_init[3];
	u8_t  data_chan_map[5];

	u8_t  data_chan_count:6;
	u8_t  data_chan_sel:1;
	u8_t  rfu:1;

	union {
		struct {
			u8_t data_chan_hop;
			u8_t data_chan_use;
		};

		u16_t data_chan_id;
	};

	u16_t handle;
	u16_t event_counter;
	u16_t conn_interval;
	u16_t latency;
	u16_t latency_prepare;
	u16_t latency_event;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH)
	u16_t default_tx_octets;
	u16_t max_tx_octets;
	u16_t max_rx_octets;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
	u8_t phy_pref_tx:3;
	u8_t phy_tx:3;
	u8_t phy_pref_flags:1;
	u8_t phy_flags:1;
	u8_t phy_pref_rx:3;
	u8_t phy_rx:3;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */

	u16_t supervision_reload;
	u16_t supervision_expire;
	u16_t procedure_reload;
	u16_t procedure_expire;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
	u16_t appto_reload;
	u16_t appto_expire;
	u16_t apto_reload;
	u16_t apto_expire;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

	union {
		struct {
			u8_t  role:1;
			u8_t  connect_expire;
		} master;
		struct {
			u8_t  role:1;
			u8_t  sca:3;
			u8_t  latency_cancel:1;
			u32_t window_widening_periodic_us;
			u32_t window_widening_max_us;
			u32_t window_widening_prepare_us;
			u32_t window_widening_event_us;
			u32_t window_size_prepare_us;
			u32_t window_size_event_us;
			u32_t force;
			u32_t ticks_to_offset;
		} slave;
	} role;

	u8_t  llcp_req;
	u8_t  llcp_ack;
	enum  llcp llcp_type;
	union {
		struct {
			u16_t interval;
			u16_t latency;
			u16_t timeout;
			u8_t  preferred_periodicity;
			u16_t instant;
			u16_t offset0;
			u16_t offset1;
			u16_t offset2;
			u16_t offset3;
			u16_t offset4;
			u16_t offset5;
			u32_t ticks_ref;
			u32_t ticks_to_offset_next;
			u32_t win_offset_us;
			u16_t *pdu_win_offset;
			u8_t  win_size;
			u8_t  state:3;
#define LLCP_CONN_STATE_INPROG    0	/* master + slave proc in progress
					 * until instant
					 */
#define LLCP_CONN_STATE_INITIATE  1	/* master sends conn_update */
#define LLCP_CONN_STATE_REQ       2	/* master / slave send req */
#define LLCP_CONN_STATE_RSP       3	/* master rej / slave rej/rsp */
#define LLCP_CONN_STATE_APP_WAIT  4	/* app resp */
#define LLCP_CONN_STATE_RSP_WAIT  5	/* master rsp or slave conn_update
					 * or rej
					 */
			u8_t  is_internal:2;
		} connection_update;
		struct {
			u8_t  initiate;
			u8_t  chm[5];
			u16_t instant;
		} chan_map;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
		struct {
			u8_t initiate:1;
			u8_t cmd:1;
			u8_t tx:3;
			u8_t rx:3;
			u16_t instant;
		} phy_upd_ind;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */

		struct {
			u8_t  error_code;
			u8_t  rand[8];
			u8_t  ediv[2];
			u8_t  ltk[16];
			u8_t  skd[16];
		} encryption;
	} llcp;

	u32_t llcp_features;

	struct {
		u8_t  tx:1;
		u8_t  rx:1;
		u8_t  version_number;
		u16_t company_id;
		u16_t sub_version_number;
	} llcp_version;

	struct {
		u8_t  req;
		u8_t  ack;
		u8_t  reason_own;
		u8_t  reason_peer;
		struct {
			struct radio_pdu_node_rx_hdr hdr;
			u8_t reason;
		} radio_pdu_node_rx;
	} llcp_terminate;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH)
	struct {
		u8_t  req;
		u8_t  ack;
		u8_t  state:2;
#define LLCP_LENGTH_STATE_REQ        0
#define LLCP_LENGTH_STATE_ACK_WAIT   1
#define LLCP_LENGTH_STATE_RSP_WAIT   2
#define LLCP_LENGTH_STATE_RESIZE     3
		u16_t rx_octets;
		u16_t tx_octets;
	} llcp_length;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
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
		u8_t flags:1;
		u8_t cmd:1;
	} llcp_phy;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */

	u8_t  sn:1;
	u8_t  nesn:1;
	u8_t  pause_rx:1;
	u8_t  pause_tx:1;
	u8_t  enc_rx:1;
	u8_t  enc_tx:1;
	u8_t  refresh:1;
	u8_t  empty:1;

	struct ccm ccm_rx;
	struct ccm ccm_tx;

	struct radio_pdu_node_tx *pkt_tx_head;
	struct radio_pdu_node_tx *pkt_tx_ctrl;
	struct radio_pdu_node_tx *pkt_tx_data;
	struct radio_pdu_node_tx *pkt_tx_last;
	u8_t  packet_tx_head_len;
	u8_t  packet_tx_head_offset;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI)
	u8_t  rssi_latest;
	u8_t  rssi_reported;
	u8_t  rssi_sample_count;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI */
};
#define CONNECTION_T_SIZE MROUND(sizeof(struct connection))

struct pdu_data_q_tx {
	u16_t  handle;
	struct radio_pdu_node_tx *node_tx;
};

/* Minimum Rx Data allocation size */
#define PACKET_RX_DATA_SIZE_MIN \
			MROUND(offsetof(struct radio_pdu_node_rx, pdu_data) + \
			(PDU_AC_SIZE_MAX + 1))

/* Minimum Tx Ctrl allocation size */
#define PACKET_TX_CTRL_SIZE_MIN \
			MROUND(offsetof(struct radio_pdu_node_tx, pdu_data) + \
			offsetof(struct pdu_data, payload) + 27)

/** @todo fix starvation when ctrl rx in radio ISR
 * for multiple connections needs to tx back to peer.
 */
#define PACKET_MEM_COUNT_TX_CTRL	2

#define LL_MEM_CONN (sizeof(struct connection) * RADIO_CONNECTION_CONTEXT_MAX)

#define LL_MEM_RXQ (sizeof(void *) * (RADIO_PACKET_COUNT_RX_MAX + 4))
#define LL_MEM_TXQ (sizeof(struct pdu_data_q_tx) * \
		    (RADIO_PACKET_COUNT_TX_MAX + 2))

#define LL_MEM_RX_POOL_SZ (MROUND(offsetof(struct radio_pdu_node_rx,\
				pdu_data) + ((\
			(PDU_AC_SIZE_MAX + 1) < \
			 (offsetof(struct pdu_data, payload) + \
			  RADIO_LL_LENGTH_OCTETS_RX_MAX)) ? \
		      (offsetof(struct pdu_data, payload) + \
		      RADIO_LL_LENGTH_OCTETS_RX_MAX) \
			: \
		      (PDU_AC_SIZE_MAX + 1))) * \
			(RADIO_PACKET_COUNT_RX_MAX + 3))

#define LL_MEM_RX_LINK_POOL (sizeof(void *) * 2 * ((RADIO_PACKET_COUNT_RX_MAX +\
				4) + RADIO_CONNECTION_CONTEXT_MAX))

#define LL_MEM_TX_CTRL_POOL (PACKET_TX_CTRL_SIZE_MIN * PACKET_MEM_COUNT_TX_CTRL)
#define LL_MEM_TX_DATA_POOL ((MROUND(offsetof( \
					struct radio_pdu_node_tx, pdu_data) + \
		   offsetof(struct pdu_data, payload) + \
				RADIO_PACKET_TX_DATA_SIZE)) \
			* (RADIO_PACKET_COUNT_TX_MAX + 1))

#define LL_MEM_TOTAL (LL_MEM_CONN + LL_MEM_RXQ + (LL_MEM_TXQ * 2) + \
		LL_MEM_RX_POOL_SZ + \
		LL_MEM_RX_LINK_POOL + LL_MEM_TX_CTRL_POOL + LL_MEM_TX_DATA_POOL)
