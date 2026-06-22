/* l2cap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

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
#define BTP_L2CAP_CONNECTION_RESPONSE_INSUFF_SEC_AUTHEN	0x05

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

#define BTP_L2CAP_SEND_ECHO_REQ				0x0a
struct btp_l2cap_send_echo_req_cmd {
	bt_addr_le_t address;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_L2CAP_CONNECT_V2_MODE_NONE			0x00
#define BTP_L2CAP_CONNECT_V2_MODE_BASIC			BTP_L2CAP_CONNECT_V2_MODE_NONE
#define BTP_L2CAP_CONNECT_V2_MODE_RET			0x01
#define BTP_L2CAP_CONNECT_V2_MODE_FC			0x02
#define BTP_L2CAP_CONNECT_V2_MODE_ERET			0x03
#define BTP_L2CAP_CONNECT_V2_MODE_STREAM		0x04

#define BTP_L2CAP_CONNECT_V2_OPT_ECFC			BTP_L2CAP_CONNECT_OPT_ECFC
#define BTP_L2CAP_CONNECT_V2_OPT_HOLD_CREDIT		BTP_L2CAP_CONNECT_OPT_HOLD_CREDIT
#define BTP_L2CAP_CONNECT_V2_OPT_MODE_OPTIONAL		BIT(2)
#define BTP_L2CAP_CONNECT_V2_OPT_EXT_WIN_SIZE		BIT(3)
#define BTP_L2CAP_CONNECT_V2_OPT_NO_FCS			BIT(4)

#define BTP_L2CAP_CONNECT_V2				0x0b
struct btp_l2cap_connect_v2_cmd {
	bt_addr_le_t address;
	uint16_t psm;
	uint16_t mtu;
	uint8_t num;
	uint8_t mode;
	uint32_t options;
} __packed;

struct btp_l2cap_connect_v2_rp {
	uint8_t num;
	uint8_t chan_id[];
} __packed;

#define BTP_L2CAP_LISTEN_V2_MODE_NONE			0x00
#define BTP_L2CAP_LISTEN_V2_MODE_BASIC			BTP_L2CAP_LISTEN_V2_MODE_NONE
#define BTP_L2CAP_LISTEN_V2_MODE_RET			0x01
#define BTP_L2CAP_LISTEN_V2_MODE_FC			0x02
#define BTP_L2CAP_LISTEN_V2_MODE_ERET			0x03
#define BTP_L2CAP_LISTEN_V2_MODE_STREAM			0x04
#define BTP_L2CAP_LISTEN_V2_MODE_VALID			BTP_L2CAP_LISTEN_V2_MODE_STREAM

#define BTP_L2CAP_LISTEN_V2_OPT_ECFC			BIT(0)
#define BTP_L2CAP_LISTEN_V2_OPT_HOLD_CREDIT		BIT(1)
#define BTP_L2CAP_LISTEN_V2_OPT_MODE_OPTIONAL		BIT(2)
#define BTP_L2CAP_LISTEN_V2_OPT_EXT_WIN_SIZE		BIT(3)
#define BTP_L2CAP_LISTEN_V2_OPT_NO_FCS			BIT(4)

#define BTP_L2CAP_LISTEN_V2				0x0c
struct btp_l2cap_listen_v2_cmd {
	uint16_t psm;
	uint8_t transport;
	uint16_t mtu;
	uint16_t response;
	uint8_t mode;
	uint32_t options;
} __packed;

#define BTP_L2CAP_CONNLESS_SEND				0x0d
struct btp_l2cap_connless_send_cmd {
	bt_addr_le_t address;
	uint16_t psm;
	uint32_t options;
	uint16_t data_length;
	uint8_t data[];
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
