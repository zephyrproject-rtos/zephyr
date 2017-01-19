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

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
#define RADIO_BLE_FEATURES_BIT_PING BIT(BT_LE_FEAT_BIT_PING)
#else /* !CONFIG_BLUETOOTH_CONTROLLER_LE_PING */
#define RADIO_BLE_FEATURES_BIT_PING 0
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX)
#define RADIO_BLE_FEATURES_BIT_DLE BIT(BT_LE_FEAT_BIT_DLE)
#define RADIO_LL_LENGTH_OCTETS_RX_MAX \
		CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX
#else
#define RADIO_BLE_FEATURES_BIT_DLE 0
#define RADIO_LL_LENGTH_OCTETS_RX_MAX 27
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_MAX */

/*****************************************************************************
 * Timer Resources (Controller defined)
 ****************************************************************************/
#define RADIO_TICKER_ID_EVENT		 0
#define RADIO_TICKER_ID_MARKER_0	 1
#define RADIO_TICKER_ID_PRE_EMPT	 2
#define RADIO_TICKER_ID_ADV_STOP	 3
#define RADIO_TICKER_ID_OBS_STOP	 4
#define RADIO_TICKER_ID_ADV		 5
#define RADIO_TICKER_ID_OBS		 6
#define RADIO_TICKER_ID_FIRST_CONNECTION 7

#define RADIO_TICKER_INSTANCE_ID_RADIO	 0
#define RADIO_TICKER_INSTANCE_ID_APP	 1

#define RADIO_TICKER_USERS		 3

#define RADIO_TICKER_USER_ID_WORKER	 TICKER_MAYFLY_CALL_ID_WORKER0
#define RADIO_TICKER_USER_ID_JOB	 TICKER_MAYFLY_CALL_ID_JOB0
#define RADIO_TICKER_USER_ID_APP	 TICKER_MAYFLY_CALL_ID_PROGRAM

#define RADIO_TICKER_USER_ID_WORKER_PRIO TICKER_MAYFLY_CALL_ID_WORKER0_PRIO
#define RADIO_TICKER_USER_ID_JOB_PRIO    TICKER_MAYFLY_CALL_ID_JOB0_PRIO

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
#define RADIO_BLE_VERSION_NUMBER	(0x08)
#define RADIO_BLE_COMPANY_ID		(0xFFFF)
#define RADIO_BLE_SUB_VERSION_NUMBER	(0xFFFF)

#define RADIO_BLE_FEATURES              (BIT(BT_LE_FEAT_BIT_ENC) | \
					 BIT(BT_LE_FEAT_BIT_CONN_PARAM_REQ) | \
					 BIT(BT_LE_FEAT_BIT_EXT_REJ_IND) | \
					 BIT(BT_LE_FEAT_BIT_SLAVE_FEAT_REQ) | \
					 RADIO_BLE_FEATURES_BIT_PING | \
					 RADIO_BLE_FEATURES_BIT_DLE)

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
	uint8_t data[DOUBLE_BUFFER_SIZE][RADIO_ACPDU_SIZE_MAX];
	uint8_t first;
	uint8_t last;
};

struct radio_le_conn_cmplt {
	uint8_t status;
	uint8_t role;
	uint8_t peer_addr_type;
	uint8_t peer_addr[BDADDR_SIZE];
	uint8_t own_addr_type;
	uint8_t own_addr[BDADDR_SIZE];
	uint8_t peer_irk_index;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint8_t mca;
} __packed;

struct radio_le_conn_update_cmplt {
	uint8_t status;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
} __packed;

struct radio_pdu_node_tx {
	void *next;
	uint8_t pdu_data[1];
};

enum radio_pdu_node_rx_type {
	NODE_RX_TYPE_NONE,
	NODE_RX_TYPE_DC_PDU,
	NODE_RX_TYPE_REPORT,
	NODE_RX_TYPE_CONNECTION,
	NODE_RX_TYPE_TERMINATE,
	NODE_RX_TYPE_CONN_UPDATE,
	NODE_RX_TYPE_ENC_REFRESH,

#if defined(CONFIG_BLUETOOTH_CONTROLLER_LE_PING)
	NODE_RX_TYPE_APTO,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_LE_PING */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI)
	NODE_RX_TYPE_RSSI,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
	NODE_RX_TYPE_PROFILE,
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */
};

struct radio_pdu_node_rx_hdr {
	union {
		void *next; /* used also by k_fifo once pulled */
		void *link;
		uint8_t packet_release_last;
	} onion;

	enum radio_pdu_node_rx_type type;
	uint16_t handle;
};

struct radio_pdu_node_rx {
	struct radio_pdu_node_rx_hdr hdr;
	uint8_t pdu_data[1];
};

/*****************************************************************************
 * Controller Interface Functions
 ****************************************************************************/
uint32_t radio_init(void *hf_clock, uint8_t sca, uint8_t connection_count_max,
		    uint8_t rx_count_max, uint8_t tx_count_max,
		    uint16_t packet_data_octets_max,
		    uint16_t packet_tx_data_size, uint8_t *mem_radio,
		    uint16_t mem_size);
void ctrl_reset(void);
void radio_ticks_active_to_start_set(uint32_t ticks_active_to_start);
struct radio_adv_data *radio_adv_data_get(void);
struct radio_adv_data *radio_scan_data_get(void);
void radio_filter_clear(void);
uint32_t radio_filter_add(uint8_t addr_type, uint8_t *addr);
uint32_t radio_filter_remove(uint8_t addr_type, uint8_t *addr);
void radio_irk_clear(void);
uint32_t radio_irk_add(uint8_t *irk);
uint32_t radio_adv_enable(uint16_t interval, uint8_t chl_map,
		uint8_t filter_policy);
uint32_t radio_adv_disable(void);
uint32_t radio_scan_enable(uint8_t scan_type, uint8_t init_addr_type,
		uint8_t *init_addr, uint16_t interval,
		uint16_t window, uint8_t filter_policy);
uint32_t radio_scan_disable(void);
uint32_t radio_connect_enable(uint8_t adv_addr_type, uint8_t *adv_addr,
		uint16_t interval, uint16_t latency,
			      uint16_t timeout);
uint32_t radio_connect_disable(void);
uint32_t radio_conn_update(uint16_t handle, uint8_t cmd, uint8_t status,
		uint16_t interval, uint16_t latency,
		uint16_t timeout);
uint32_t radio_chm_update(uint8_t *chm);
uint32_t radio_chm_get(uint16_t handle, uint8_t *chm);
uint32_t radio_enc_req_send(uint16_t handle, uint8_t *rand, uint8_t *ediv,
		uint8_t *ltk);
uint32_t radio_start_enc_req_send(uint16_t handle, uint8_t err_code,
		uint8_t const *const ltk);
uint32_t radio_feature_req_send(uint16_t handle);
uint32_t radio_version_ind_send(uint16_t handle);
uint32_t radio_terminate_ind_send(uint16_t handle, uint8_t reason);

#if defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH)
uint32_t radio_length_req_send(uint16_t handle, uint16_t tx_octets);
void radio_length_default_get(uint16_t *max_tx_octets, uint16_t *max_tx_time);
uint32_t radio_length_default_set(uint16_t max_tx_octets, uint16_t max_tx_time);
void radio_length_max_get(uint16_t *max_tx_octets, uint16_t *max_tx_time,
			  uint16_t *max_rx_octets, uint16_t *max_rx_time);
#endif /* CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH */

uint8_t radio_rx_get(struct radio_pdu_node_rx **radio_pdu_node_rx,
		uint16_t *handle);
void radio_rx_dequeue(void);
void radio_rx_mem_release(struct radio_pdu_node_rx **radio_pdu_node_rx);
uint8_t radio_rx_fc_set(uint16_t handle, uint8_t fc);
uint8_t radio_rx_fc_get(uint16_t *handle);
struct radio_pdu_node_tx *radio_tx_mem_acquire(void);
void radio_tx_mem_release(struct radio_pdu_node_tx *pdu_data_node_tx);
uint32_t radio_tx_mem_enqueue(uint16_t handle,
		struct radio_pdu_node_tx *pdu_data_node_tx);

extern void radio_active_callback(uint8_t active);
extern void radio_event_callback(void);

#endif
