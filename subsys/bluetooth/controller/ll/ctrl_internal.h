/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include "util/defines.h"
#include "pdu.h"
#include "ctrl.h"

enum llcp {
	LLCP_NONE,
	LLCP_CONNECTION_UPDATE,
	LLCP_CHANNEL_MAP,
	LLCP_ENCRYPTION,
	LLCP_FEATURE_EXCHANGE,
	LLCP_VERSION_EXCHANGE,
	/* LLCP_TERMINATE, */
	LLCP_PING,
	/* LLCP_LENGTH, */
};


struct shdr {
	uint32_t ticks_xtal_to_start;
	uint32_t ticks_active_to_start;
	uint32_t ticks_preempt_to_start;
	uint32_t ticks_slot;
};

struct connection {
	struct shdr hdr;

	uint8_t access_addr[4];
	uint8_t crc_init[3];
	uint8_t data_channel_map[5];
	uint8_t data_channel_count;
	uint8_t data_channel_hop;
	uint8_t data_channel_use;
	uint16_t handle;
	uint16_t event_counter;
	uint16_t conn_interval;
	uint16_t latency;
	uint16_t latency_prepare;
	uint16_t latency_event;
	uint16_t sug_tx_octets;
	uint16_t max_tx_octets;
	uint16_t max_rx_octets;
	uint16_t supervision_reload;
	uint16_t supervision_expire;
	uint16_t procedure_reload;
	uint16_t procedure_expire;
	uint16_t appto_reload;
	uint16_t appto_expire;
	uint16_t apto_reload;
	uint16_t apto_expire;

	union {
		struct {
			uint8_t role:1;
			uint8_t connect_expire;
		} master;
		struct {
			uint8_t role:1;
			uint8_t sca:3;
			uint8_t latency_cancel:1;
			uint32_t window_widening_periodic_us;
			uint32_t window_widening_max_us;
			uint32_t window_widening_prepare_us;
			uint32_t window_widening_event_us;
			uint32_t window_size_prepare_us;
			uint32_t window_size_event_us;
			uint32_t force;
			uint32_t ticks_to_offset;
		} slave;
	} role;

	uint8_t llcp_req;
	uint8_t llcp_ack;
	enum llcp llcp_type;
	union {
		struct {
			uint16_t interval;
			uint16_t latency;
			uint16_t timeout;
			uint8_t preferred_periodicity;
			uint16_t instant;
			uint16_t offset0;
			uint16_t offset1;
			uint16_t offset2;
			uint16_t offset3;
			uint16_t offset4;
			uint16_t offset5;
			uint32_t ticks_ref;
			uint32_t ticks_to_offset_next;
			uint32_t win_offset_us;
			uint16_t *pdu_win_offset;
			uint8_t win_size;
			uint8_t state:3;
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
			uint8_t is_internal:2;
		} connection_update;
		struct {
			uint8_t initiate;
			uint8_t chm[5];
			uint16_t instant;
		} channel_map;
		struct {
			uint8_t error_code;
			uint8_t rand[8];
			uint8_t ediv[2];
			uint8_t ltk[16];
			uint8_t skd[16];
		} encryption;
	} llcp;

	uint8_t llcp_features;

	struct {
		uint8_t tx:1;
		uint8_t rx:1;
		uint8_t version_number;
		uint16_t company_id;
		uint16_t sub_version_number;
	} llcp_version;

	struct {
		uint8_t req;
		uint8_t ack;
		uint8_t reason_own;
		uint8_t reason_peer;
		struct {
			struct radio_pdu_node_rx_hdr hdr;
			uint8_t reason;
		} radio_pdu_node_rx;
	} llcp_terminate;

	struct {
		uint8_t req;
		uint8_t ack;
		uint8_t state:2;
#define LLCP_LENGTH_STATE_REQ        0
#define LLCP_LENGTH_STATE_ACK_WAIT   1
#define LLCP_LENGTH_STATE_RSP_WAIT   2
#define LLCP_LENGTH_STATE_RESIZE     3
		uint16_t rx_octets;
		uint16_t tx_octets;
	} llcp_length;

	uint8_t sn:1;
	uint8_t nesn:1;
	uint8_t pause_rx:1;
	uint8_t pause_tx:1;
	uint8_t enc_rx:1;
	uint8_t enc_tx:1;
	uint8_t refresh:1;
	uint8_t empty:1;

	struct ccm ccm_rx;
	struct ccm ccm_tx;

	struct radio_pdu_node_tx *pkt_tx_head;
	struct radio_pdu_node_tx *pkt_tx_ctrl;
	struct radio_pdu_node_tx *pkt_tx_data;
	struct radio_pdu_node_tx *pkt_tx_last;
	uint8_t packet_tx_head_len;
	uint8_t packet_tx_head_offset;

	uint8_t rssi_latest;
	uint8_t rssi_reported;
	uint8_t rssi_sample_count;
};
#define CONNECTION_T_SIZE ALIGN4(sizeof(struct connection))

struct pdu_data_q_tx {
	uint16_t handle;
	struct radio_pdu_node_tx *node_tx;
};

/** @todo fix starvation when ctrl rx in radio ISR
 * for multiple connections needs to tx back to peer.
 */
#define PACKET_MEM_COUNT_TX_CTRL	2

#define LL_MEM_CONN (sizeof(struct connection) * RADIO_CONNECTION_CONTEXT_MAX)

#define LL_MEM_RXQ (sizeof(void *) * (RADIO_PACKET_COUNT_RX_MAX + 4))
#define LL_MEM_TXQ (sizeof(struct pdu_data_q_tx) * \
		    (RADIO_PACKET_COUNT_TX_MAX + 2))

#define LL_MEM_RX_POOL_SZ (ALIGN4(offsetof(struct radio_pdu_node_rx,\
				pdu_data) + ((\
			(RADIO_ACPDU_SIZE_MAX + 1) < \
			 (offsetof(struct pdu_data, payload) + \
			  RADIO_LL_LENGTH_OCTETS_RX_MAX)) ? \
		      (offsetof(struct pdu_data, payload) + \
		      RADIO_LL_LENGTH_OCTETS_RX_MAX) \
			: \
		      (RADIO_ACPDU_SIZE_MAX + 1))) * \
			(RADIO_PACKET_COUNT_RX_MAX + 3))

#define LL_MEM_RX_LINK_POOL (sizeof(void *) * 2 * ((RADIO_PACKET_COUNT_RX_MAX +\
				4) + RADIO_CONNECTION_CONTEXT_MAX))

#define LL_MEM_TX_CTRL_POOL ((ALIGN4(offsetof( \
					struct radio_pdu_node_tx, pdu_data) + \
		   offsetof(struct pdu_data, payload) + 27)) * \
		PACKET_MEM_COUNT_TX_CTRL)
#define LL_MEM_TX_DATA_POOL ((ALIGN4(offsetof( \
					struct radio_pdu_node_tx, pdu_data) + \
		   offsetof(struct pdu_data, payload) + \
				RADIO_LL_LENGTH_OCTETS_RX_MAX)) \
			* (RADIO_PACKET_COUNT_TX_MAX + 1))

#define LL_MEM_TOTAL (LL_MEM_CONN + LL_MEM_RXQ + (LL_MEM_TXQ * 2) + \
		LL_MEM_RX_POOL_SZ + \
		LL_MEM_RX_LINK_POOL + LL_MEM_TX_CTRL_POOL + LL_MEM_TX_DATA_POOL)
