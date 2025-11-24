/* btp_rfcomm.h - Bluetooth RFCOMM tester headers */

/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>

/* RFCOMM commands */
#define BTP_RFCOMM_READ_SUPPORTED_COMMANDS	0x01

struct btp_rfcomm_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_RFCOMM_CONNECT	0x02

struct btp_rfcomm_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
	uint8_t flags;
} __packed;

struct btp_rfcomm_connect_rp {
	uint8_t status;
	uint8_t dlc_state;
} __packed;

#define BTP_RFCOMM_REGISTER_SERVER       0x03

struct btp_rfcomm_register_server_cmd {
	uint8_t channel;
	uint8_t flags;
} __packed;

struct btp_rfcomm_register_server_rp {
	uint8_t status;
} __packed;

#define BTP_RFCOMM_SEND_DATA           0x04

struct btp_rfcomm_send_data_cmd {
	uint8_t data_len;
	uint8_t channel;
	uint8_t flags;
	uint8_t data[];
} __packed;

struct btp_rfcomm_send_data_rp {
	uint8_t status;
} __packed;

#define BTP_RFCOMM_DISCONNECT          0x05

struct btp_rfcomm_disconnect_cmd {
	uint8_t channel;
	uint8_t flags;
} __packed;

struct btp_rfcomm_disconnect_rp {
	uint8_t status;
} __packed;

#define BTP_RFCOMM_EV_CONNECTED        0x80

struct btp_rfcomm_connected_ev {
	uint8_t channel;
	uint16_t mtu;
	uint8_t state;
} __packed;

#define BTP_RFCOMM_EV_DISCONNECTED  0x81

struct btp_rfcomm_disconnected_ev {
	uint8_t channel;
	uint8_t state;
} __packed;

#define BTP_RFCOMM_EV_DATA_RECEIVED  0x82

struct btp_rfcomm_data_received_ev {
	uint8_t channel;
	uint16_t data_length;
	uint8_t data[];
} __packed;
