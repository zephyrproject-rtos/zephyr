/* btp_pbap.h - Bluetooth PBAP Tester */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>

/* PBAP Service */
/* Commands */
#define BTP_PBAP_READ_SUPPORTED_COMMANDS	0x01
struct btp_pbap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

/* PBAP PCE Commands */
#define BTP_PBAP_PCE_RFCOMM_CONNECT		0x02
struct btp_pbap_pce_rfcomm_connect_cmd {
	bt_addr_le_t address;
	uint8_t channel;
} __packed;

#define BTP_PBAP_PCE_RFCOMM_DISCONNECT		0x03
struct btp_pbap_pce_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_L2CAP_CONNECT		0x04
struct btp_pbap_pce_l2cap_connect_cmd {
	bt_addr_le_t address;
	uint16_t psm;
} __packed;

#define BTP_PBAP_PCE_L2CAP_DISCONNECT		0x05
struct btp_pbap_pce_l2cap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_CONNECT			0x06
struct btp_pbap_pce_connect_cmd {
	bt_addr_le_t address;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_DISCONNECT			0x07
struct btp_pbap_pce_disconnect_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_PULL_PHONEBOOK		0x08
struct btp_pbap_pce_pull_phonebook_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_PULL_VCARD_LISTING		0x09
struct btp_pbap_pce_pull_vcard_listing_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_PULL_VCARD_ENTRY		0x0a
struct btp_pbap_pce_pull_vcard_entry_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_SET_PATH			0x0b
struct btp_pbap_pce_set_path_cmd {
	bt_addr_le_t address;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_ABORT			0x0c
struct btp_pbap_pce_abort_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

/* PBAP PSE Commands */
#define BTP_PBAP_PSE_RFCOMM_REGISTER		0x10
struct btp_pbap_pse_rfcomm_register_cmd {
	/* No parameters */
} __packed;

#define BTP_PBAP_PSE_L2CAP_REGISTER		0x11
struct btp_pbap_pse_l2cap_register_cmd {
	/* No parameters */
} __packed;

#define BTP_PBAP_PSE_CONNECT_RSP		0x12
struct btp_pbap_pse_connect_rsp_cmd {
	bt_addr_le_t address;
	uint16_t mopl;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_DISCONNECT_RSP		0x13
struct btp_pbap_pse_disconnect_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_PULL_PHONEBOOK_RSP		0x14
struct btp_pbap_pse_pull_phonebook_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_PULL_VCARD_LISTING_RSP	0x15
struct btp_pbap_pse_pull_vcard_listing_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_PULL_VCARD_ENTRY_RSP	0x16
struct btp_pbap_pse_pull_vcard_entry_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_SET_PATH_RSP		0x17
struct btp_pbap_pse_set_path_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_ABORT_RSP			0x18
struct btp_pbap_pse_abort_rsp_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

/* Events */
/* PBAP PCE Events */
#define BTP_PBAP_PCE_EV_RFCOMM_CONNECTED	0x80
struct btp_pbap_pce_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_EV_RFCOMM_DISCONNECTED	0x81
struct btp_pbap_pce_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_EV_L2CAP_CONNECTED		0x82
struct btp_pbap_pce_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_EV_L2CAP_DISCONNECTED	0x83
struct btp_pbap_pce_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PCE_EV_CONNECTED		0x84
struct btp_pbap_pce_connected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_DISCONNECTED		0x85
struct btp_pbap_pce_disconnected_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_PULL_PHONEBOOK		0x86
struct btp_pbap_pce_pull_phonebook_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_PULL_VCARD_LISTING	0x87
struct btp_pbap_pce_pull_vcard_listing_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_PULL_VCARD_ENTRY	0x88
struct btp_pbap_pce_pull_vcard_entry_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_SET_PATH		0x89
struct btp_pbap_pce_set_path_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PCE_EV_ABORT			0x8a
struct btp_pbap_pce_abort_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

/* PBAP PSE Events */
#define BTP_PBAP_PSE_EV_RFCOMM_CONNECTED	0x90
struct btp_pbap_pse_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PSE_EV_RFCOMM_DISCONNECTED	0x91
struct btp_pbap_pse_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PSE_EV_L2CAP_CONNECTED		0x92
struct btp_pbap_pse_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PSE_EV_L2CAP_DISCONNECTED	0x93
struct btp_pbap_pse_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_PBAP_PSE_EV_CONNECT_REQ		0x94
struct btp_pbap_pse_connect_req_ev {
	bt_addr_le_t address;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_DISCONNECT_REQ		0x95
struct btp_pbap_pse_disconnect_req_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_PULL_PHONEBOOK_REQ	0x96
struct btp_pbap_pse_pull_phonebook_req_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_PULL_VCARD_LISTING_REQ	0x97
struct btp_pbap_pse_pull_vcard_listing_req_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_PULL_VCARD_ENTRY_REQ	0x98
struct btp_pbap_pse_pull_vcard_entry_req_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_SET_PATH_REQ		0x99
struct btp_pbap_pse_set_path_req_ev {
	bt_addr_le_t address;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;

#define BTP_PBAP_PSE_EV_ABORT_REQ		0x9a
struct btp_pbap_pse_abort_req_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[0];
} __packed;
