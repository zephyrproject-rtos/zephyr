/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PDU_H_
#define _PDU_H_

#define BDADDR_SIZE 6

/* PDU Sizes */
#define PDU_AC_PAYLOAD_SIZE_MAX 37
#define PDU_AC_SIZE_MAX (offsetof(struct pdu_adv, payload) + \
			 PDU_AC_PAYLOAD_SIZE_MAX)
#define PDU_EM_SIZE_MAX offsetof(struct pdu_data, payload)

struct pdu_adv_payload_adv_ind {
	u8_t addr[BDADDR_SIZE];
	u8_t data[31];
} __packed;

struct pdu_adv_payload_direct_ind {
	u8_t adv_addr[BDADDR_SIZE];
	u8_t tgt_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_payload_scan_rsp {
	u8_t addr[BDADDR_SIZE];
	u8_t data[31];
} __packed;

struct pdu_adv_payload_scan_req {
	u8_t scan_addr[BDADDR_SIZE];
	u8_t adv_addr[BDADDR_SIZE];
} __packed;

struct pdu_adv_payload_connect_ind {
	u8_t init_addr[BDADDR_SIZE];
	u8_t adv_addr[BDADDR_SIZE];
	struct {
		u8_t  access_addr[4];
		u8_t  crc_init[3];
		u8_t  win_size;
		u16_t win_offset;
		u16_t interval;
		u16_t latency;
		u16_t timeout;
		u8_t  chan_map[5];
		u8_t  hop:5;
		u8_t  sca:3;
	} __packed lldata;
} __packed;

enum pdu_adv_type {
	PDU_ADV_TYPE_ADV_IND = 0x00,
	PDU_ADV_TYPE_DIRECT_IND = 0x01,
	PDU_ADV_TYPE_NONCONN_IND = 0x02,
	PDU_ADV_TYPE_SCAN_REQ = 0x03,
	PDU_ADV_TYPE_AUX_SCAN_REQ = PDU_ADV_TYPE_SCAN_REQ,
	PDU_ADV_TYPE_SCAN_RSP = 0x04,
	PDU_ADV_TYPE_CONNECT_IND = 0x05,
	PDU_ADV_TYPE_AUX_CONNECT_REQ = PDU_ADV_TYPE_CONNECT_IND,
	PDU_ADV_TYPE_SCAN_IND = 0x06,
	PDU_ADV_TYPE_EXT_IND = 0x07,
	PDU_ADV_TYPE_AUX_ADV_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_SCAN_RSP = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_SYNC_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_CHAIN_IND = PDU_ADV_TYPE_EXT_IND,
	PDU_ADV_TYPE_AUX_CONNECT_RSP = 0x08,
} __packed;

struct pdu_adv {
	u8_t type:4;
	u8_t rfu:1;
	u8_t chan_sel:1;
	u8_t tx_addr:1;
	u8_t rx_addr:1;

	u8_t len:8;

	union {
		struct pdu_adv_payload_adv_ind adv_ind;
		struct pdu_adv_payload_direct_ind direct_ind;
		struct pdu_adv_payload_scan_req scan_req;
		struct pdu_adv_payload_scan_rsp scan_rsp;
		struct pdu_adv_payload_connect_ind connect_ind;
	} __packed payload;
} __packed;

enum pdu_data_llid {
	PDU_DATA_LLID_RESV = 0x00,
	PDU_DATA_LLID_DATA_CONTINUE = 0x01,
	PDU_DATA_LLID_DATA_START = 0x02,
	PDU_DATA_LLID_CTRL = 0x03,
};

enum pdu_data_llctrl_type {
	PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND = 0x00,
	PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND = 0x01,
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
	PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND = 0x11,
	PDU_DATA_LLCTRL_TYPE_PING_REQ = 0x12,
	PDU_DATA_LLCTRL_TYPE_PING_RSP = 0x13,
	PDU_DATA_LLCTRL_TYPE_LENGTH_REQ = 0x14,
	PDU_DATA_LLCTRL_TYPE_LENGTH_RSP = 0x15,
	PDU_DATA_LLCTRL_TYPE_PHY_REQ = 0x16,
	PDU_DATA_LLCTRL_TYPE_PHY_RSP = 0x17,
	PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND = 0x18,
	PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND = 0x19,
};

struct pdu_data_llctrl_conn_update_ind {
	u8_t  win_size;
	u16_t win_offset;
	u16_t interval;
	u16_t latency;
	u16_t timeout;
	u16_t instant;
} __packed;

struct pdu_data_llctrl_chan_map_ind {
	u8_t  chm[5];
	u16_t instant;
} __packed;

struct pdu_data_llctrl_terminate_ind {
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_enc_req {
	u8_t rand[8];
	u8_t ediv[2];
	u8_t skdm[8];
	u8_t ivm[4];
} __packed;

struct pdu_data_llctrl_enc_rsp {
	u8_t skds[8];
	u8_t ivs[4];
} __packed;

struct pdu_data_llctrl_unknown_rsp {
	u8_t type;
} __packed;

struct pdu_data_llctrl_feature_req {
	u8_t features[8];
} __packed;

struct pdu_data_llctrl_feature_rsp {
	u8_t features[8];
} __packed;

struct pdu_data_llctrl_version_ind {
	u8_t  version_number;
	u16_t company_id;
	u16_t sub_version_number;
} __packed;

struct pdu_data_llctrl_reject_ind {
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_conn_param_req {
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
} __packed;

struct pdu_data_llctrl_conn_param_rsp {
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
} __packed;

struct pdu_data_llctrl_reject_ext_ind {
	u8_t reject_opcode;
	u8_t error_code;
} __packed;

struct pdu_data_llctrl_length_req_rsp {
	u16_t max_rx_octets;
	u16_t max_rx_time;
	u16_t max_tx_octets;
	u16_t max_tx_time;
} __packed;

struct pdu_data_llctrl_phy_req_rsp {
	u8_t tx_phys;
	u8_t rx_phys;
} __packed;

struct pdu_data_llctrl_phy_upd_ind {
	u8_t  m_to_s_phy;
	u8_t  s_to_m_phy;
	u16_t instant;
} __packed;

struct pdu_data_llctrl_min_used_chans_ind {
	u8_t phys;
	u8_t min_used_chans;
} __packed;

struct pdu_data_llctrl {
	u8_t opcode;
	union {
		struct pdu_data_llctrl_conn_update_ind conn_update_ind;
		struct pdu_data_llctrl_chan_map_ind chan_map_ind;
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
		struct pdu_data_llctrl_reject_ext_ind reject_ext_ind;
		struct pdu_data_llctrl_length_req_rsp length_req;
		struct pdu_data_llctrl_length_req_rsp length_rsp;
		struct pdu_data_llctrl_phy_req_rsp phy_req;
		struct pdu_data_llctrl_phy_req_rsp phy_rsp;
		struct pdu_data_llctrl_phy_upd_ind phy_upd_ind;
		struct pdu_data_llctrl_min_used_chans_ind min_used_chans_ind;
	} __packed ctrldata;
} __packed;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
struct profile {
	u8_t lcur;
	u8_t lmin;
	u8_t lmax;
	u8_t cur;
	u8_t min;
	u8_t max;
} __packed;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */

struct pdu_data {
	u8_t ll_id:2;
	u8_t nesn:1;
	u8_t sn:1;
	u8_t md:1;
	u8_t rfu:3;

	u8_t len:8;

#if !defined(CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_CLEAR)
	u8_t resv:8; /* TODO: remove nRF specific code */
#endif /* !CONFIG_BLUETOOTH_CONTROLLER_DATA_LENGTH_CLEAR */

	union {
		u8_t lldata[1];
		struct pdu_data_llctrl llctrl;

#if defined(CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI)
		u8_t rssi;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_CONN_RSSI */

#if defined(CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR)
		struct profile profile;
#endif /* CONFIG_BLUETOOTH_CONTROLLER_PROFILE_ISR */
	} __packed payload;
} __packed;

#endif /* _PDU_H_ */
