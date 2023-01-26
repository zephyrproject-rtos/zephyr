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
#define L2CAP_READ_SUPPORTED_COMMANDS	0x01
struct l2cap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define L2CAP_CONNECT_OPT_ECFC		0x01
#define L2CAP_CONNECT_OPT_HOLD_CREDIT	0x02

#define L2CAP_CONNECT			0x02
struct l2cap_connect_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t psm;
	uint16_t mtu;
	uint8_t num;
	uint8_t options;
} __packed;
struct l2cap_connect_rp {
	uint8_t num;
	uint8_t chan_id[];
} __packed;

#define L2CAP_DISCONNECT		0x03
struct l2cap_disconnect_cmd {
	uint8_t chan_id;
} __packed;

#define L2CAP_SEND_DATA			0x04
struct l2cap_send_data_cmd {
	uint8_t chan_id;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define L2CAP_TRANSPORT_BREDR		0x00
#define L2CAP_TRANSPORT_LE		0x01

#define L2CAP_CONNECTION_RESPONSE_SUCCESS		0x00
#define L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHEN		0x01
#define L2CAP_CONNECTION_RESPONSE_INSUFF_AUTHOR		0x02
#define L2CAP_CONNECTION_RESPONSE_INSUFF_ENC_KEY		0x03

#define L2CAP_LISTEN			0x05
struct l2cap_listen_cmd {
	uint16_t psm;
	uint8_t transport;
	uint16_t mtu;
	uint16_t response;
} __packed;

#define L2CAP_ACCEPT_CONNECTION		0x06
struct l2cap_accept_connection_cmd {
	uint8_t chan_id;
	uint16_t result;
} __packed;

#define L2CAP_RECONFIGURE		0x07
struct l2cap_reconfigure_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint16_t mtu;
	uint8_t num;
	uint8_t chan_id[];
} __packed;

#define L2CAP_CREDITS		0x08
struct l2cap_credits_cmd {
	uint8_t chan_id;
} __packed;

#define L2CAP_DISCONNECT_EATT_CHANS		0x09
struct l2cap_disconnect_eatt_chans_cmd {
	uint8_t address_type;
	uint8_t address[6];
	uint8_t count;
} __packed;

/* events */
#define L2CAP_EV_CONNECTION_REQ		0x80
struct l2cap_connection_req_ev {
	uint8_t chan_id;
	uint16_t psm;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_CONNECTED		0x81
struct l2cap_connected_ev {
	uint8_t chan_id;
	uint16_t psm;
	uint16_t mtu_remote;
	uint16_t mps_remote;
	uint16_t mtu_local;
	uint16_t mps_local;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_DISCONNECTED		0x82
struct l2cap_disconnected_ev {
	uint16_t result;
	uint8_t chan_id;
	uint16_t psm;
	uint8_t address_type;
	uint8_t address[6];
} __packed;

#define L2CAP_EV_DATA_RECEIVED		0x83
struct l2cap_data_received_ev {
	uint8_t chan_id;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define L2CAP_EV_RECONFIGURED		0x84
struct l2cap_reconfigured_ev {
	uint8_t chan_id;
	uint16_t mtu_remote;
	uint16_t mps_remote;
	uint16_t mtu_local;
	uint16_t mps_local;
} __packed;
