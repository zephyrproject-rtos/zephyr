/* btp_map.h - Bluetooth tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/addr.h>

/* MAP Service */
/* commands */
#define BTP_MAP_READ_SUPPORTED_COMMANDS 0x01
struct btp_map_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

/* MAP Client MAS commands */
#define BTP_MAP_MCE_MAS_RFCOMM_CONNECT 0x02
struct btp_map_mce_mas_rfcomm_connect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t channel;
} __packed;

#define BTP_MAP_MCE_MAS_RFCOMM_DISCONNECT 0x03
struct btp_map_mce_mas_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_L2CAP_CONNECT 0x04
struct btp_map_mce_mas_l2cap_connect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint16_t psm;
} __packed;

#define BTP_MAP_MCE_MAS_L2CAP_DISCONNECT 0x05
struct btp_map_mce_mas_l2cap_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_CONNECT 0x06
struct btp_map_mce_mas_connect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t send_supp_feat;
} __packed;

#define BTP_MAP_MCE_MAS_DISCONNECT 0x07
struct btp_map_mce_mas_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_ABORT 0x08
struct btp_map_mce_mas_abort_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_SET_FOLDER 0x09
struct btp_map_mce_mas_set_folder_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_SET_NTF_REG 0x0a
struct btp_map_mce_mas_set_ntf_reg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_FOLDER_LISTING 0x0b
struct btp_map_mce_mas_get_folder_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_MSG_LISTING 0x0c
struct btp_map_mce_mas_get_msg_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_MSG 0x0d
struct btp_map_mce_mas_get_msg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_SET_MSG_STATUS 0x0e
struct btp_map_mce_mas_set_msg_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_PUSH_MSG 0x0f
struct btp_map_mce_mas_push_msg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_UPDATE_INBOX 0x10
struct btp_map_mce_mas_update_inbox_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_MAS_INST_INFO 0x11
struct btp_map_mce_mas_get_mas_inst_info_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_SET_OWNER_STATUS 0x12
struct btp_map_mce_mas_set_owner_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_OWNER_STATUS 0x13
struct btp_map_mce_mas_get_owner_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_GET_CONVO_LISTING 0x14
struct btp_map_mce_mas_get_convo_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_SET_NTF_FILTER 0x15
struct btp_map_mce_mas_set_ntf_filter_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Client MNS commands */
#define BTP_MAP_MCE_MNS_RFCOMM_DISCONNECT 0x16
struct btp_map_mce_mns_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_L2CAP_DISCONNECT 0x17
struct btp_map_mce_mns_l2cap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_CONNECT 0x18
struct btp_map_mce_mns_connect_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MCE_MNS_DISCONNECT 0x19
struct btp_map_mce_mns_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MCE_MNS_ABORT 0x1a
struct btp_map_mce_mns_abort_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MCE_MNS_SEND_EVENT 0x1b
struct btp_map_mce_mns_send_event_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Server MAS commands */
#define BTP_MAP_MSE_MAS_RFCOMM_DISCONNECT 0x1c
struct btp_map_mse_mas_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_L2CAP_DISCONNECT 0x1d
struct btp_map_mse_mas_l2cap_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_CONNECT 0x1e
struct btp_map_mse_mas_connect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MSE_MAS_DISCONNECT 0x1f
struct btp_map_mse_mas_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MSE_MAS_ABORT 0x20
struct btp_map_mse_mas_abort_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
} __packed;

#define BTP_MAP_MSE_MAS_SET_FOLDER 0x21
struct btp_map_mse_mas_set_folder_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_SET_NTF_REG 0x22
struct btp_map_mse_mas_set_ntf_reg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_FOLDER_LISTING 0x23
struct btp_map_mse_mas_get_folder_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_MSG_LISTING 0x24
struct btp_map_mse_mas_get_msg_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_MSG 0x25
struct btp_map_mse_mas_get_msg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_SET_MSG_STATUS 0x26
struct btp_map_mse_mas_set_msg_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_PUSH_MSG 0x27
struct btp_map_mse_mas_push_msg_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_UPDATE_INBOX 0x28
struct btp_map_mse_mas_update_inbox_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_MAS_INST_INFO 0x29
struct btp_map_mse_mas_get_mas_inst_info_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_SET_OWNER_STATUS 0x2a
struct btp_map_mse_mas_set_owner_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_OWNER_STATUS 0x2b
struct btp_map_mse_mas_get_owner_status_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_GET_CONVO_LISTING 0x2c
struct btp_map_mse_mas_get_convo_listing_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_SET_NTF_FILTER 0x2d
struct btp_map_mse_mas_set_ntf_filter_cmd {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Server MNS commands */
#define BTP_MAP_MSE_MNS_RFCOMM_CONNECT 0x2e
struct btp_map_mse_mns_rfcomm_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_MAP_MSE_MNS_RFCOMM_DISCONNECT 0x2f
struct btp_map_mse_mns_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_L2CAP_CONNECT 0x30
struct btp_map_mse_mns_l2cap_connect_cmd {
	bt_addr_le_t address;
	uint16_t psm;
} __packed;

#define BTP_MAP_MSE_MNS_L2CAP_DISCONNECT 0x31
struct btp_map_mse_mns_l2cap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_CONNECT 0x32
struct btp_map_mse_mns_connect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_DISCONNECT 0x33
struct btp_map_mse_mns_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_ABORT 0x34
struct btp_map_mse_mns_abort_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_SEND_EVENT 0x35
struct btp_map_mse_mns_send_event_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_SDP_DISCOVER 0x36
struct btp_map_sdp_discover_cmd {
	bt_addr_le_t address;
	uint16_t uuid;
} __packed;

/* events */
/* MAP Client MAS events */
#define BTP_MAP_MCE_MAS_EV_RFCOMM_CONNECTED 0x80
struct btp_map_mce_mas_rfcomm_connected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_EV_RFCOMM_DISCONNECTED 0x81
struct btp_map_mce_mas_rfcomm_disconnected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_EV_L2CAP_CONNECTED 0x82
struct btp_map_mce_mas_l2cap_connected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_EV_L2CAP_DISCONNECTED 0x83
struct btp_map_mce_mas_l2cap_disconnected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MCE_MAS_EV_CONNECT 0x84
struct btp_map_mce_mas_connect_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_DISCONNECT 0x85
struct btp_map_mce_mas_disconnect_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_ABORT 0x86
struct btp_map_mce_mas_abort_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_SET_NTF_REG 0x87
struct btp_map_mce_mas_set_ntf_reg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_SET_FOLDER 0x88
struct btp_map_mce_mas_set_folder_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_FOLDER_LISTING 0x89
struct btp_map_mce_mas_get_folder_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_MSG_LISTING 0x8a
struct btp_map_mce_mas_get_msg_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_MSG 0x8b
struct btp_map_mce_mas_get_msg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_SET_MSG_STATUS 0x8c
struct btp_map_mce_mas_set_msg_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_PUSH_MSG 0x8d
struct btp_map_mce_mas_push_msg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_UPDATE_INBOX 0x8e
struct btp_map_mce_mas_update_inbox_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_MAS_INST_INFO 0x8f
struct btp_map_mce_mas_get_mas_inst_info_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_SET_OWNER_STATUS 0x90
struct btp_map_mce_mas_set_owner_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_OWNER_STATUS 0x91
struct btp_map_mce_mas_get_owner_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_GET_CONVO_LISTING 0x92
struct btp_map_mce_mas_get_convo_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MAS_EV_SET_NTF_FILTER 0x93
struct btp_map_mce_mas_set_ntf_filter_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Client MNS events */
#define BTP_MAP_MCE_MNS_EV_RFCOMM_CONNECTED 0x94
struct btp_map_mce_mns_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_EV_RFCOMM_DISCONNECTED 0x95
struct btp_map_mce_mns_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_EV_L2CAP_CONNECTED 0x96
struct btp_map_mce_mns_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_EV_L2CAP_DISCONNECTED 0x97
struct btp_map_mce_mns_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MCE_MNS_EV_CONNECT 0x98
struct btp_map_mce_mns_connect_ev {
	bt_addr_le_t address;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MNS_EV_DISCONNECT 0x99
struct btp_map_mce_mns_disconnect_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MNS_EV_ABORT 0x9a
struct btp_map_mce_mns_abort_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MCE_MNS_EV_SEND_EVENT 0x9b
struct btp_map_mce_mns_send_event_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Server MAS events */
#define BTP_MAP_MSE_MAS_EV_RFCOMM_CONNECTED 0x9c
struct btp_map_mse_mas_rfcomm_connected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_EV_RFCOMM_DISCONNECTED 0x9d
struct btp_map_mse_mas_rfcomm_disconnected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_EV_L2CAP_CONNECTED 0x9e
struct btp_map_mse_mas_l2cap_connected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_EV_L2CAP_DISCONNECTED 0x9f
struct btp_map_mse_mas_l2cap_disconnected_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
} __packed;

#define BTP_MAP_MSE_MAS_EV_CONNECT 0xa0
struct btp_map_mse_mas_connect_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_DISCONNECT 0xa1
struct btp_map_mse_mas_disconnect_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_ABORT 0xa2
struct btp_map_mse_mas_abort_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_SET_NTF_REG 0xa3
struct btp_map_mse_mas_set_ntf_reg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_SET_FOLDER 0xa4
struct btp_map_mse_mas_set_folder_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_FOLDER_LISTING 0xa5
struct btp_map_mse_mas_get_folder_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_MSG_LISTING 0xa6
struct btp_map_mse_mas_get_msg_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_MSG 0xa7
struct btp_map_mse_mas_get_msg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_SET_MSG_STATUS 0xa8
struct btp_map_mse_mas_set_msg_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_PUSH_MSG 0xa9
struct btp_map_mse_mas_push_msg_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_UPDATE_INBOX 0xaa
struct btp_map_mse_mas_update_inbox_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_MAS_INST_INFO 0xab
struct btp_map_mse_mas_get_mas_inst_info_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_SET_OWNER_STATUS 0xac
struct btp_map_mse_mas_set_owner_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_OWNER_STATUS 0xad
struct btp_map_mse_mas_get_owner_status_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_GET_CONVO_LISTING 0xae
struct btp_map_mse_mas_get_convo_listing_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MAS_EV_SET_NTF_FILTER 0xaf
struct btp_map_mse_mas_set_ntf_filter_ev {
	bt_addr_le_t address;
	uint8_t instance_id;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* MAP Server MNS events */
#define BTP_MAP_MSE_MNS_EV_RFCOMM_CONNECTED 0xb0
struct btp_map_mse_mns_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_EV_RFCOMM_DISCONNECTED 0xb1
struct btp_map_mse_mns_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_EV_L2CAP_CONNECTED 0xb2
struct btp_map_mse_mns_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_EV_L2CAP_DISCONNECTED 0xb3
struct btp_map_mse_mns_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_MAP_MSE_MNS_EV_CONNECT 0xb4
struct btp_map_mse_mns_connect_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MNS_EV_DISCONNECT 0xb5
struct btp_map_mse_mns_disconnect_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MNS_EV_ABORT 0xb6
struct btp_map_mse_mns_abort_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_MSE_MNS_EV_SEND_EVENT 0xb7
struct btp_map_mse_mns_send_event_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_MAP_EV_SDP_RECORD 0xb8
struct btp_map_sdp_record_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t uuid;
	uint8_t instance_id;
	uint8_t rfcomm_channel;
	uint16_t l2cap_psm;
	uint16_t version;
	uint32_t supported_features;
	uint8_t msg_types;
	uint8_t service_name_len;
	uint8_t service_name[];
} __packed;
