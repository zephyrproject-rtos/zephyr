/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CTRL_H_
#define _CTRL_H_

/*****************************************************************************
 * Zephyr Kconfig defined
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_MAX_CONN
#define RADIO_CONNECTION_CONTEXT_MAX CONFIG_BLUETOOTH_MAX_CONN
#else
#define RADIO_CONNECTION_CONTEXT_MAX 0
#endif

#ifdef CONFIG_BLUETOOTH_CONTROLLER_RX_BUFFERS
#define RADIO_PACKET_COUNT_RX_MAX \
		CONFIG_BLUETOOTH_CONTROLLER_RX_BUFFERS
#endif

#ifdef CONFIG_BLUETOOTH_CONTROLLER_TX_BUFFERS
#define RADIO_PACKET_COUNT_TX_MAX \
		CONFIG_BLUETOOTH_CONTROLLER_TX_BUFFERS
#endif

#ifdef CONFIG_BLUETOOTH_CONTROLLER_TX_BUFFER_SIZE
#define RADIO_PACKET_TX_DATA_SIZE \
		CONFIG_BLUETOOTH_CONTROLLER_TX_BUFFER_SIZE
#endif

#define BIT64(n) (1ULL << (n))

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_ENC)
#define RADIO_BLE_FEAT_BIT_ENC BIT64(BT_LE_FEAT_BIT_ENC)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_LE_ENC */
#define RADIO_BLE_FEAT_BIT_ENC 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_LE_ENC */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
#define RADIO_BLE_FEAT_BIT_PING BIT64(BT_LE_FEAT_BIT_PING)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_LE_PING */
#define RADIO_BLE_FEAT_BIT_PING 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX)
#define RADIO_BLE_FEAT_BIT_DLE BIT64(BT_LE_FEAT_BIT_DLE)
#define RADIO_LL_LENGTH_OCTETS_RX_MAX \
		CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX
#else
#define RADIO_BLE_FEAT_BIT_DLE 0
#define RADIO_LL_LENGTH_OCTETS_RX_MAX 27
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2)
#define RADIO_BLE_FEAT_BIT_CHAN_SEL_2 BIT64(BT_LE_FEAT_BIT_CHAN_SEL_ALGO_2)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2 */
#define RADIO_BLE_FEAT_BIT_CHAN_SEL_2 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_CHAN_SEL_2 */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY_2M)
#define RADIO_BLE_FEAT_BIT_PHY_2M BIT64(BT_LE_FEAT_BIT_PHY_2M)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_PHY_2M */
#define RADIO_BLE_FEAT_BIT_PHY_2M 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_PHY_2M */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY_CODED)
#define RADIO_BLE_FEAT_BIT_PHY_CODED BIT64(BT_LE_FEAT_BIT_PHY_CODED)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_PHY_CODED */
#define RADIO_BLE_FEAT_BIT_PHY_CODED 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_PHY_CODED */

/*****************************************************************************
 * Timer Resources (Controller defined)
 ****************************************************************************/
#define RADIO_TICKER_ID_EVENT		 0
#define RADIO_TICKER_ID_MARKER_0	 1
#define RADIO_TICKER_ID_PRE_EMPT	 2
#define RADIO_TICKER_ID_ADV_STOP	 3
#define RADIO_TICKER_ID_SCAN_STOP	 4
#define RADIO_TICKER_ID_ADV		 5
#define RADIO_TICKER_ID_SCAN		 6
#define RADIO_TICKER_ID_FIRST_CONNECTION 7

#define RADIO_TICKER_INSTANCE_ID_RADIO	 0
#define RADIO_TICKER_INSTANCE_ID_APP	 1

#define RADIO_TICKER_USERS		 3

#define RADIO_TICKER_USER_ID_WORKER	 MAYFLY_CALL_ID_0
#define RADIO_TICKER_USER_ID_JOB	 MAYFLY_CALL_ID_1
#define RADIO_TICKER_USER_ID_APP	 MAYFLY_CALL_ID_PROGRAM

#define RADIO_TICKER_USER_WORKER_OPS	(7 + 1)
#define RADIO_TICKER_USER_JOB_OPS	(2 + 1)
#define RADIO_TICKER_USER_APP_OPS	(1 + 1)
#define RADIO_TICKER_USER_OPS		(RADIO_TICKER_USER_WORKER_OPS \
					+ RADIO_TICKER_USER_JOB_OPS \
					+ RADIO_TICKER_USER_APP_OPS \
					)

#define RADIO_TICKER_NODES		(RADIO_TICKER_ID_FIRST_CONNECTION \
					+ RADIO_CONNECTION_CONTEXT_MAX \
					)

/*****************************************************************************
 * Controller Interface Defines
 ****************************************************************************/
#define RADIO_BLE_VERSION_NUMBER	BT_HCI_VERSION_5_0
#if defined(CONFIG_BLUETOOTH_CONTROLLER_COMPANY_ID)
#define RADIO_BLE_COMPANY_ID            CONFIG_BLUETOOTH_CONTROLLER_COMPANY_ID
#else
#define RADIO_BLE_COMPANY_ID            0xFFFF
#endif
#if defined(CONFIG_BLUETOOTH_CONTROLLER_SUBVERSION_NUMBER)
#define RADIO_BLE_SUB_VERSION_NUMBER \
				CONFIG_BLUETOOTH_CONTROLLER_SUBVERSION_NUMBER
#else
#define RADIO_BLE_SUB_VERSION_NUMBER    0xFFFF
#endif

#define RADIO_BLE_FEAT_BIT_MASK         0x1FFFF
#define RADIO_BLE_FEAT_BIT_MASK_VALID   0x1CF2F
#define RADIO_BLE_FEAT                  (RADIO_BLE_FEAT_BIT_ENC | \
					 BIT(BT_LE_FEAT_BIT_CONN_PARAM_REQ) | \
					 BIT(BT_LE_FEAT_BIT_EXT_REJ_IND) | \
					 BIT(BT_LE_FEAT_BIT_SLAVE_FEAT_REQ) | \
					 RADIO_BLE_FEAT_BIT_PING | \
					 RADIO_BLE_FEAT_BIT_DLE | \
					 RADIO_BLE_FEAT_BIT_PHY_2M | \
					 RADIO_BLE_FEAT_BIT_PHY_CODED | \
					 RADIO_BLE_FEAT_BIT_CHAN_SEL_2)

#if defined(CONFIG_BLUETOOTH_CONTROLLER_WORKER_PRIO)
#define RADIO_TICKER_USER_ID_WORKER_PRIO CONFIG_BLUETOOTH_CONTROLLER_WORKER_PRIO
#else
#define RADIO_TICKER_USER_ID_WORKER_PRIO 0
#endif

#if defined(CONFIG_BLUETOOTH_CONTROLLER_JOB_PRIO)
#define RADIO_TICKER_USER_ID_JOB_PRIO CONFIG_BLUETOOTH_CONTROLLER_JOB_PRIO
#else
#define RADIO_TICKER_USER_ID_JOB_PRIO 0
#endif

/*****************************************************************************
 * Controller Reference Defines (compile time override-able)
 ****************************************************************************/
/* Minimum LL Payload support (Dont change). */
#define RADIO_LL_LENGTH_OCTETS_RX_MIN	27
#define RADIO_LL_LENGTH_TIME_RX_MIN	(((RADIO_LL_LENGTH_OCTETS_RX_MIN) \
						+ 14) * 8 \
					)

/* Maximum LL Payload support (27 to 251). */
#ifndef RADIO_LL_LENGTH_OCTETS_RX_MAX
#define RADIO_LL_LENGTH_OCTETS_RX_MAX	251
#endif
#define RADIO_LL_LENGTH_TIME_RX_MAX	(((RADIO_LL_LENGTH_OCTETS_RX_MAX) \
						+ 14) * 8 \
					)
/* Implementation default L2CAP MTU */
#ifndef RADIO_L2CAP_MTU_MAX
#define RADIO_L2CAP_MTU_MAX		(RADIO_LL_LENGTH_OCTETS_RX_MAX - 4)
#endif

/* Maximise L2CAP MTU to LL data PDU size */
#if (RADIO_L2CAP_MTU_MAX < (RADIO_LL_LENGTH_OCTETS_RX_MAX - 4))
#undef RADIO_L2CAP_MTU_MAX
#define RADIO_L2CAP_MTU_MAX		(RADIO_LL_LENGTH_OCTETS_RX_MAX - 4)
#endif

/* Maximum LL PDU Receive pool size. */
#ifndef RADIO_PACKET_COUNT_RX_MAX
#define RADIO_PACKET_COUNT_RX		((RADIO_L2CAP_MTU_MAX + \
						RADIO_LL_LENGTH_OCTETS_RX_MAX \
						+ 3) \
						/ \
						RADIO_LL_LENGTH_OCTETS_RX_MAX \
					)
#define RADIO_PACKET_COUNT_RX_MAX	(RADIO_PACKET_COUNT_RX + \
					((RADIO_CONNECTION_CONTEXT_MAX - 1) * \
					(RADIO_PACKET_COUNT_RX - 1)) \
					)
#endif /* RADIO_PACKET_COUNT_RX_MAX */

/* Maximum LL PDU Transmit pool size and application tx count. */
#ifndef RADIO_PACKET_COUNT_TX_MAX
#define RADIO_PACKET_COUNT_APP_TX_MAX	(RADIO_CONNECTION_CONTEXT_MAX)
#define RADIO_PACKET_COUNT_TX_MAX	(RADIO_PACKET_COUNT_RX_MAX + \
						RADIO_PACKET_COUNT_APP_TX_MAX \
					)
#else
#define RADIO_PACKET_COUNT_APP_TX_MAX	(RADIO_PACKET_COUNT_TX_MAX)
#endif

/* Tx Data Size */
#if !defined(RADIO_PACKET_TX_DATA_SIZE) || \
	(RADIO_PACKET_TX_DATA_SIZE < RADIO_LL_LENGTH_OCTETS_RX_MIN)
#define RADIO_PACKET_TX_DATA_SIZE RADIO_LL_LENGTH_OCTETS_RX_MIN
#endif

/*****************************************************************************
 * Controller Interface Structures
 ****************************************************************************/
struct radio_adv_data {
	u8_t data[DOUBLE_BUFFER_SIZE][PDU_AC_SIZE_MAX];
	u8_t first;
	u8_t last;
};

struct radio_pdu_node_tx {
	void *next;
	u8_t pdu_data[1];
};

enum radio_pdu_node_rx_type {
	NODE_RX_TYPE_NONE,
	NODE_RX_TYPE_DC_PDU,
	NODE_RX_TYPE_REPORT,

#if defined(CONFIG_BLUETOOTH_CONTROLLER_SCAN_REQ_NOTIFY)
	NODE_RX_TYPE_SCAN_REQ,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_SCAN_REQ_NOTIFY */

	NODE_RX_TYPE_CONNECTION,
	NODE_RX_TYPE_TERMINATE,
	NODE_RX_TYPE_CONN_UPDATE,
	NODE_RX_TYPE_ENC_REFRESH,

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
	NODE_RX_TYPE_APTO,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

	NODE_RX_TYPE_CHAN_SEL_ALGO,

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PHY)
	NODE_RX_TYPE_PHY_UPDATE,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PHY */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI)
	NODE_RX_TYPE_RSSI,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
	NODE_RX_TYPE_PROFILE,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_ADV_INDICATION)
	NODE_RX_TYPE_ADV_INDICATION,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_ADV_INDICATION */
};

struct radio_le_conn_cmplt {
	u8_t  status;
	u8_t  role;
	u8_t  peer_addr_type;
	u8_t  peer_addr[BDADDR_SIZE];
	u8_t  own_addr_type;
	u8_t  own_addr[BDADDR_SIZE];
	u8_t  peer_irk_index;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
	u8_t  mca;
} __packed;

struct radio_le_conn_update_cmplt {
	u8_t  status;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
} __packed;

struct radio_le_chan_sel_algo {
	u8_t chan_sel_algo;
} __packed;

struct radio_le_phy_upd_cmplt {
	u8_t status;
	u8_t tx;
	u8_t rx;
} __packed;

struct radio_pdu_node_rx_hdr {
	union {
		sys_snode_t node; /* used by slist */
		void *next; /* used also by k_fifo once pulled */
		void *link;
		u8_t packet_release_last;
	} onion;

	enum radio_pdu_node_rx_type type;
	u16_t handle;
};

struct radio_pdu_node_rx {
	struct radio_pdu_node_rx_hdr hdr;
	u8_t   pdu_data[1];
};

/*****************************************************************************
 * Controller Interface Functions
 ****************************************************************************/
/* Downstream */
u32_t radio_init(void *hf_clock, u8_t sca, u8_t connection_count_max,
		 u8_t rx_count_max, u8_t tx_count_max,
		 u16_t packet_data_octets_max,
		 u16_t packet_tx_data_size, u8_t *mem_radio,
		 u16_t mem_size);
void radio_ticks_active_to_start_set(u32_t ticks_active_to_start);
/* Downstream - Advertiser */
struct radio_adv_data *radio_adv_data_get(void);
struct radio_adv_data *radio_scan_data_get(void);
u32_t radio_adv_enable(u16_t interval, u8_t chl_map,
		       u8_t filter_policy);
u32_t radio_adv_disable(void);
u32_t radio_adv_is_enabled(void);
u32_t radio_adv_filter_pol_get(void);
/* Downstream - Scanner */
u32_t radio_scan_enable(u8_t type, u8_t init_addr_type,
			u8_t *init_addr, u16_t interval,
			u16_t window, u8_t filter_policy);
u32_t radio_scan_disable(void);
u32_t radio_scan_is_enabled(void);
u32_t radio_scan_filter_pol_get(void);

u32_t radio_connect_enable(u8_t adv_addr_type, u8_t *adv_addr,
			   u16_t interval, u16_t latency,
			   u16_t timeout);
/* Upstream */
u8_t radio_rx_get(struct radio_pdu_node_rx **radio_pdu_node_rx,
		  u16_t *handle);
void radio_rx_dequeue(void);
void radio_rx_mem_release(struct radio_pdu_node_rx **radio_pdu_node_rx);
u8_t radio_rx_fc_set(u16_t handle, u8_t fc);
u8_t radio_rx_fc_get(u16_t *handle);
struct radio_pdu_node_tx *radio_tx_mem_acquire(void);
void radio_tx_mem_release(struct radio_pdu_node_tx *pdu_data_node_tx);
u32_t radio_tx_mem_enqueue(u16_t handle,
			   struct radio_pdu_node_tx *pdu_data_node_tx);
/* Callbacks */
extern void radio_active_callback(u8_t active);
extern void radio_event_callback(void);

#endif
