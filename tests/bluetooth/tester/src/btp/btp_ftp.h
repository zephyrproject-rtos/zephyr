/* btp_ftp.h - Bluetooth tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/addr.h>

/* FTP Service */
/* Commands */
#define BTP_FTP_READ_SUPPORTED_COMMANDS 0x01
struct btp_ftp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

/* FTP Client commands */
#define BTP_FTP_CLIENT_RFCOMM_CONNECT 0x02
struct btp_ftp_client_rfcomm_connect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_RFCOMM_DISCONNECT 0x03
struct btp_ftp_client_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_L2CAP_CONNECT 0x04
struct btp_ftp_client_l2cap_connect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_L2CAP_DISCONNECT 0x05
struct btp_ftp_client_l2cap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_CONNECT 0x06
struct btp_ftp_client_connect_cmd {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_DISCONNECT 0x07
struct btp_ftp_client_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_ABORT 0x08
struct btp_ftp_client_abort_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_SET_FOLDER 0x09
struct btp_ftp_client_set_folder_cmd {
	bt_addr_le_t address;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_PULL_FOLDER_LISTING 0x0a
struct btp_ftp_client_pull_folder_listing_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_PUSH_FILE 0x0b
struct btp_ftp_client_push_file_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_PULL_FILE 0x0c
struct btp_ftp_client_pull_file_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_DELETE 0x0d
struct btp_ftp_client_delete_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_RENAME 0x0e
struct btp_ftp_client_rename_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_COPY 0x0f
struct btp_ftp_client_copy_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_SET_PERMISSION 0x10
struct btp_ftp_client_set_permission_cmd {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* FTP Server commands */
#define BTP_FTP_SERVER_RFCOMM_DISCONNECT 0x11
struct btp_ftp_server_rfcomm_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_L2CAP_DISCONNECT 0x12
struct btp_ftp_server_l2cap_disconnect_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_CONNECT 0x13
struct btp_ftp_server_connect_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_DISCONNECT 0x14
struct btp_ftp_server_disconnect_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_ABORT 0x15
struct btp_ftp_server_abort_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_SET_FOLDER 0x16
struct btp_ftp_server_set_folder_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_PULL_FOLDER_LISTING 0x17
struct btp_ftp_server_pull_folder_listing_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_PUSH_FILE 0x18
struct btp_ftp_server_push_file_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_PULL_FILE 0x19
struct btp_ftp_server_pull_file_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_DELETE 0x1a
struct btp_ftp_server_delete_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_RENAME 0x1b
struct btp_ftp_server_rename_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_COPY 0x1c
struct btp_ftp_server_copy_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

#define BTP_FTP_SERVER_SET_PERMISSION 0x1d
struct btp_ftp_server_set_permission_cmd {
	bt_addr_le_t address;
	uint8_t rsp_code;
} __packed;

/* Events */
/* FTP Client events */
#define BTP_FTP_CLIENT_EV_RFCOMM_CONNECTED 0x80
struct btp_ftp_client_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_EV_RFCOMM_DISCONNECTED 0x81
struct btp_ftp_client_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_EV_L2CAP_CONNECTED 0x82
struct btp_ftp_client_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_EV_L2CAP_DISCONNECTED 0x83
struct btp_ftp_client_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_CLIENT_EV_CONNECT 0x84
struct btp_ftp_client_connect_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_DISCONNECT 0x85
struct btp_ftp_client_disconnect_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_ABORT 0x86
struct btp_ftp_client_abort_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_SET_FOLDER 0x87
struct btp_ftp_client_set_folder_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_PULL_FOLDER_LISTING 0x88
struct btp_ftp_client_pull_folder_listing_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_PUSH_FILE 0x89
struct btp_ftp_client_push_file_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_PULL_FILE 0x8a
struct btp_ftp_client_pull_file_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_DELETE 0x8b
struct btp_ftp_client_delete_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_RENAME 0x8c
struct btp_ftp_client_rename_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_COPY 0x8d
struct btp_ftp_client_copy_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_CLIENT_EV_SET_PERMISSION 0x8e
struct btp_ftp_client_set_permission_ev {
	bt_addr_le_t address;
	uint8_t rsp_code;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

/* FTP Server events */
#define BTP_FTP_SERVER_EV_RFCOMM_CONNECTED 0x8f
struct btp_ftp_server_rfcomm_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_EV_RFCOMM_DISCONNECTED 0x90
struct btp_ftp_server_rfcomm_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_EV_L2CAP_CONNECTED 0x91
struct btp_ftp_server_l2cap_connected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_EV_L2CAP_DISCONNECTED 0x92
struct btp_ftp_server_l2cap_disconnected_ev {
	bt_addr_le_t address;
} __packed;

#define BTP_FTP_SERVER_EV_CONNECT 0x93
struct btp_ftp_server_connect_ev {
	bt_addr_le_t address;
	uint8_t version;
	uint16_t mopl;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_DISCONNECT 0x94
struct btp_ftp_server_disconnect_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_ABORT 0x95
struct btp_ftp_server_abort_ev {
	bt_addr_le_t address;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_SET_FOLDER 0x96
struct btp_ftp_server_set_folder_ev {
	bt_addr_le_t address;
	uint8_t flags;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_PULL_FOLDER_LISTING 0x97
struct btp_ftp_server_pull_folder_listing_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_PUSH_FILE 0x98
struct btp_ftp_server_push_file_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_PULL_FILE 0x99
struct btp_ftp_server_pull_file_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_DELETE 0x9a
struct btp_ftp_server_delete_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_RENAME 0x9b
struct btp_ftp_server_rename_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_COPY 0x9c
struct btp_ftp_server_copy_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;

#define BTP_FTP_SERVER_EV_SET_PERMISSION 0x9d
struct btp_ftp_server_set_permission_ev {
	bt_addr_le_t address;
	uint8_t final;
	uint16_t buf_len;
	uint8_t buf[];
} __packed;
