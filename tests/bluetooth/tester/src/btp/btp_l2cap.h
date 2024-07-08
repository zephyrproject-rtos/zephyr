/* l2cap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

/* L2CAP Service */
/* commands */
#define BTP_L2CAP_READ_SUPPORTED_COMMANDS		0x01
struct btp_l2cap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_L2CAP_CONNECT_OPT_ECFC			0x01
#define BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT		0x02

#define BTP_L2CAP_CONNECT				0x02
struct btp_l2cap_connect_cmd {
	bt_addr_le_t address;
	uint16_t psm;
	uint16_t mtu;
	uint8_t num;
	uint8_t options;
} __packed;
struct btp_l2cap_connect_rp {
	uint8_t num;
	uint8_t chan_id[];
} __packed;

#define BTP_L2CAP_DISCONNECT				0x03
struct btp_l2cap_disconnect_cmd {
	uint8_t chan_id;
} __packed;

#define BTP_L2CAP_SEND_DATA				0x04
struct btp_l2cap_send_data_cmd {
	uint8_t chan_id;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_L2CAP_TRANSPORT_BREDR			0x00
#define BTP_L2CAP_TRANSPORT_LE				0x01

#define BTP_L2CAP_CONNECTION_RESPONSE_SUCCESS		0x00
#define BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHEN	0x01
#define BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHOR	0x02
#define BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_ENC_KEY	0x03
#define BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_ENCRYPTION 0x04

#define BTP_L2CAP_LISTEN				0x05
struct btp_l2cap_listen_cmd {
	uint16_t psm;
	uint8_t transport;
	uint16_t mtu;
	uint16_t response;
} __packed;

#define BTP_L2CAP_ACCEPT_CONNECTION			0x06
struct btp_l2cap_accept_connection_cmd {
	uint8_t chan_id;
	uint16_t result;
} __packed;

#define BTP_L2CAP_RECONFIGURE				0x07
struct btp_l2cap_reconfigure_cmd {
	bt_addr_le_t address;
	uint16_t mtu;
	uint8_t num;
	uint8_t chan_id[];
} __packed;

#define BTP_L2CAP_CREDITS				0x08
struct btp_l2cap_credits_cmd {
	uint8_t chan_id;
} __packed;

#define BTP_L2CAP_DISCONNECT_EATT_CHANS			0x09
struct btp_l2cap_disconnect_eatt_chans_cmd {
	bt_addr_le_t address;
	uint8_t count;
} __packed;

#define BTP_L2CAP_CONNECT_SEC_LEVEL_0		0
#define BTP_L2CAP_CONNECT_SEC_LEVEL_1		1
#define BTP_L2CAP_CONNECT_SEC_LEVEL_2		2
#define BTP_L2CAP_CONNECT_SEC_LEVEL_3		3
#define BTP_L2CAP_CONNECT_SEC_LEVEL_4		4

#define BTP_L2CAP_CONNECT_WITH_SEC_LEVEL		0x0a
struct btp_l2cap_connect_with_sec_level_cmd {
	struct btp_l2cap_connect_cmd cmd;
	uint8_t sec_level;
} __packed;

#define BTP_L2CAP_ECHO							0x0b
struct btp_l2cap_echo_cmd {
	bt_addr_le_t address;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_L2CAP_CLS_LISTEN					0x0c
struct btp_l2cap_cls_listen_cmd {
	uint16_t psm;
} __packed;

#define BTP_L2CAP_CLS_SEND						0x0d
struct btp_l2cap_cls_send_cmd {
	bt_addr_le_t address;
	uint16_t psm;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_L2CAP_MODE_BASIC 0x00
#define BTP_L2CAP_MODE_RET 0x01
#define BTP_L2CAP_MODE_FC 0x02
#define BTP_L2CAP_MODE_ERET 0x03
#define BTP_L2CAP_MODE_STREAM 0x04

#define BTP_L2CAP_LISTEN_WITH_MODE				0x0e
struct btp_l2cap_listen_with_mode_cmd {
	uint16_t psm;
	uint8_t transport;
	uint8_t mode;
	uint16_t mtu;
	uint16_t response;
} __packed;

/* events */
#define BTP_L2CAP_EV_CONNECTION_REQ			0x80
struct btp_l2cap_connection_req_ev {
	uint8_t chan_id;
	uint16_t psm;
	bt_addr_le_t address;
} __packed;

#define BTP_L2CAP_EV_CONNECTED				0x81
struct btp_l2cap_connected_ev {
	uint8_t chan_id;
	uint16_t psm;
	uint16_t mtu_remote;
	uint16_t mps_remote;
	uint16_t mtu_local;
	uint16_t mps_local;
	bt_addr_le_t address;
} __packed;

#define BTP_L2CAP_EV_DISCONNECTED			0x82
struct btp_l2cap_disconnected_ev {
	uint16_t result;
	uint8_t chan_id;
	uint16_t psm;
	bt_addr_le_t address;
} __packed;

#define BTP_L2CAP_EV_DATA_RECEIVED			0x83
struct btp_l2cap_data_received_ev {
	uint8_t chan_id;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_L2CAP_EV_RECONFIGURED			0x84
struct btp_l2cap_reconfigured_ev {
	uint8_t chan_id;
	uint16_t mtu_remote;
	uint16_t mps_remote;
	uint16_t mtu_local;
	uint16_t mps_local;
} __packed;
