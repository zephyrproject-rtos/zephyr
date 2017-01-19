/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PDU_H_
#define _PDU_H_

#define BDADDR_SIZE 6

struct pdu_adv_payload_adv_ind {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[31];
} __packed;

struct pdu_adv_payload_direct_ind {
	uint8_t adv_addr[BDADDR_SIZE];
	uint8_t init_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_payload_scan_resp {
	uint8_t addr[BDADDR_SIZE];
	uint8_t data[31];
} __packed;

struct pdu_adv_payload_scan_req {
	uint8_t scan_addr[BDADDR_SIZE];
	uint8_t adv_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_payload_connect_req {
	uint8_t init_addr[BDADDR_SIZE];
	uint8_t adv_addr[BDADDR_SIZE];
	struct {
		uint8_t access_addr[4];
		uint8_t crc_init[3];
		uint8_t win_size;
		uint16_t win_offset;
		uint16_t interval;
		uint16_t latency;
		uint16_t timeout;
		uint8_t channel_map[5];
		uint8_t hop:5;
		uint8_t sca:3;
	} __packed lldata;
} __packed;

enum pdu_adv_type {
	PDU_ADV_TYPE_ADV_IND = 0x00,
	PDU_ADV_TYPE_DIRECT_IND = 0x01,
	PDU_ADV_TYPE_NONCONN_IND = 0x02,
	PDU_ADV_TYPE_SCAN_REQ = 0x03,
	PDU_ADV_TYPE_SCAN_RESP = 0x04,
	PDU_ADV_TYPE_CONNECT_REQ = 0x05,
	PDU_ADV_TYPE_SCAN_IND = 0x06,
} __packed;

struct pdu_adv {
	uint8_t type:4;
	uint8_t rfu0:2;
	uint8_t tx_addr:1;
	uint8_t rx_addr:1;
	uint8_t len:6;
	uint8_t rfu1:2;
	uint8_t nrf_radio_s1;
	union {
		struct pdu_adv_payload_adv_ind adv_ind;
		struct pdu_adv_payload_direct_ind direct_ind;
		struct pdu_adv_payload_scan_req scan_req;
		struct pdu_adv_payload_scan_resp scan_resp;
		struct pdu_adv_payload_connect_req connect_req;
	} __packed payload;
} __packed;

enum pdu_data_llid {
	PDU_DATA_LLID_RESV = 0x00,
	PDU_DATA_LLID_DATA_CONTINUE = 0x01,
	PDU_DATA_LLID_DATA_START = 0x02,
	PDU_DATA_LLID_CTRL = 0x03,
};

enum pdu_data_llctrl_type {
	PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_REQ = 0x00,
	PDU_DATA_LLCTRL_TYPE_CHANNEL_MAP_REQ = 0x01,
	PDU_DATA_LLCTRL_TYPE_TERMINATE_IND = 0x02,
	PDU_DATA_LLCTRL_TYPE_ENC_REQ = 0x03,
	PDU_DATA_LLCTRL_TYPE_ENC_RSP = 0x04,
	PDU_DATA_LLCTRL_TYPE_START_ENC_REQ = 0x05,
	PDU_DATA_LLCTRL_TYPE_START_ENC_RSP = 0x06,
	PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP = 0x07,
	PDU_DATA_LLCTRL_TYPE_FEATURE_REQ = 0x08,
	PDU_DATA_LLCTRL_TYPE_FEATURE_RSP = 0x09,
	PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ = 0x0A,
	PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP = 0x0B,
	PDU_DATA_LLCTRL_TYPE_VERSION_IND = 0x0C,
	PDU_DATA_LLCTRL_TYPE_REJECT_IND = 0x0D,
	PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ = 0x0E,
	PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ = 0x0F,
	PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP = 0x10,
	PDU_DATA_LLCTRL_TYPE_REJECT_IND_EXT = 0x11,
	PDU_DATA_LLCTRL_TYPE_PING_REQ = 0x12,
	PDU_DATA_LLCTRL_TYPE_PING_RSP = 0x13,
	PDU_DATA_LLCTRL_TYPE_LENGTH_REQ = 0x14,
	PDU_DATA_LLCTRL_TYPE_LENGTH_RSP = 0x15,
};

struct pdu_data_llctrl_conn_update_req {
	uint8_t win_size;
	uint16_t win_offset;
	uint16_t interval;
	uint16_t latency;
	uint16_t timeout;
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_channel_map_req {
	uint8_t chm[5];
	uint16_t instant;
} __packed;

struct pdu_data_llctrl_terminate_ind {
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_enc_req {
	uint8_t rand[8];
	uint8_t ediv[2];
	uint8_t skdm[8];
	uint8_t ivm[4];
} __packed;

struct pdu_data_llctrl_enc_rsp {
	uint8_t skds[8];
	uint8_t ivs[4];
} __packed;

struct pdu_data_llctrl_unknown_rsp {
	uint8_t type;
} __packed;

struct pdu_data_llctrl_feature_req {
	uint8_t features[8];
} __packed;

struct pdu_data_llctrl_feature_rsp {
	uint8_t features[8];
} __packed;

struct pdu_data_llctrl_version_ind {
	uint8_t version_number;
	uint16_t company_id;
	uint16_t sub_version_number;
} __packed;

struct pdu_data_llctrl_reject_ind {
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_conn_param_req {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint8_t preferred_periodicity;
	uint16_t reference_conn_event_count;
	uint16_t offset0;
	uint16_t offset1;
	uint16_t offset2;
	uint16_t offset3;
	uint16_t offset4;
	uint16_t offset5;
} __packed;

struct pdu_data_llctrl_conn_param_rsp {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint8_t preferred_periodicity;
	uint16_t reference_conn_event_count;
	uint16_t offset0;
	uint16_t offset1;
	uint16_t offset2;
	uint16_t offset3;
	uint16_t offset4;
	uint16_t offset5;
} __packed;

struct pdu_data_llctrl_reject_ind_ext {
	uint8_t reject_opcode;
	uint8_t error_code;
} __packed;

struct pdu_data_llctrl_length_req_rsp {
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
} __packed;

struct pdu_data_llctrl {
	uint8_t opcode;
	union {
		struct pdu_data_llctrl_conn_update_req conn_update_req;
		struct pdu_data_llctrl_channel_map_req channel_map_req;
		struct pdu_data_llctrl_terminate_ind terminate_ind;
		struct pdu_data_llctrl_enc_req enc_req;
		struct pdu_data_llctrl_enc_rsp enc_rsp;
		struct pdu_data_llctrl_unknown_rsp unknown_rsp;
		struct pdu_data_llctrl_feature_req feature_req;
		struct pdu_data_llctrl_feature_rsp feature_rsp;
		struct pdu_data_llctrl_version_ind version_ind;
		struct pdu_data_llctrl_reject_ind reject_ind;
		struct pdu_data_llctrl_feature_req slave_feature_req;
		struct pdu_data_llctrl_conn_param_req conn_param_req;
		struct pdu_data_llctrl_conn_param_rsp conn_param_rsp;
		struct pdu_data_llctrl_reject_ind_ext reject_ind_ext;
		struct pdu_data_llctrl_length_req_rsp length_req;
		struct pdu_data_llctrl_length_req_rsp length_rsp;
	} __packed ctrldata;
} __packed;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
struct profile {
	uint8_t lcur;
	uint8_t lmin;
	uint8_t lmax;
	uint8_t cur;
	uint8_t min;
	uint8_t max;
} __packed;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */

struct pdu_data {
	uint8_t ll_id:2;
	uint8_t nesn:1;
	uint8_t sn:1;
	uint8_t md:1;
	uint8_t rfu0:3;

	uint8_t len:8;

	uint8_t resv:8;

	union {
		uint8_t lldata[1];
		struct pdu_data_llctrl llctrl;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI)
		uint8_t rssi;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
		struct profile profile;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */
	} __packed payload;
} __packed;

#endif /* _PDU_H_ */
