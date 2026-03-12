/* btp_rfcomm.h - Bluetooth RFCOMM tester headers */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* RFCOMM Service */
/* commands */
#define BTP_RFCOMM_READ_SUPPORTED_COMMANDS		0x01
struct btp_rfcomm_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_RFCOMM_CONNECT				0x02
struct btp_rfcomm_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_RFCOMM_DISCONNECT				0x03
struct btp_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_RFCOMM_SEND_DATA				0x04
struct btp_rfcomm_send_data_cmd {
	bt_addr_le_t address;
	uint8_t channel;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_RFCOMM_LISTEN				0x05
struct btp_rfcomm_listen_cmd {
	uint8_t channel;
} __packed;

#define BTP_RFCOMM_SEND_RPN				0x06
struct btp_rfcomm_send_rpn_cmd {
	bt_addr_le_t address;
	uint8_t channel;
	uint8_t baud_rate;
	uint8_t line_settings;
	uint8_t flow_control;
	uint8_t xon_char;
	uint8_t xoff_char;
	uint16_t param_mask;
} __packed;

#define BTP_RFCOMM_GET_DLC_INFO				0x07
struct btp_rfcomm_get_dlc_info_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;
struct btp_rfcomm_get_dlc_info_rp {
	uint16_t mtu;
	uint8_t state;
} __packed;

/* events */
#define BTP_RFCOMM_EV_CONNECTED				0x80
struct btp_rfcomm_connected_ev {
	bt_addr_le_t address;
	uint8_t channel;
	uint16_t mtu;
} __packed;

#define BTP_RFCOMM_EV_DISCONNECTED			0x81
struct btp_rfcomm_disconnected_ev {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_RFCOMM_EV_DATA_RECEIVED			0x82
struct btp_rfcomm_data_received_ev {
	bt_addr_le_t address;
	uint8_t channel;
	uint16_t data_length;
	uint8_t data[];
} __packed;

#define BTP_RFCOMM_EV_DATA_SENT				0x83
struct btp_rfcomm_data_sent_ev {
	bt_addr_le_t address;
	uint8_t channel;
	int8_t err;
} __packed;
