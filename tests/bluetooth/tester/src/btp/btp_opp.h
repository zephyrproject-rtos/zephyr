/* btp_opp.h - Bluetooth OPP tester headers */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define BTP_OPP_READ_SUPPORTED_COMMANDS         0x01
struct btp_opp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

/* 0x10: Discover the OPP server on the connected peer via SDP.
 * Result reported via BTP_OPP_EV_DISCOVERED. No parameters.
 */
#define BTP_OPP_DISCOVER                        0x10

/* Client: 0x11 - 0x18 */

/* 0x11: Connect RFCOMM transport. channel: RFCOMM channel from SDP. */
#define BTP_OPP_CLIENT_TRANSPORT_CONNECT        0x11
struct btp_opp_client_transport_connect_cmd {
	uint8_t channel;
} __packed;

/* 0x12: Disconnect RFCOMM transport. No parameters. */
#define BTP_OPP_CLIENT_TRANSPORT_DISCONNECT     0x12

/* 0x13: Establish OBEX session. */
#define BTP_OPP_CLIENT_CONNECT                  0x13
struct btp_opp_client_connect_cmd {
	uint16_t mopl;
} __packed;

/* 0x14: Terminate OBEX session. No parameters. */
#define BTP_OPP_CLIENT_DISCONNECT               0x14

/* 0x15: Push an object to the server. */
#define BTP_OPP_CLIENT_PUSH                     0x15
struct btp_opp_client_push_cmd {
	uint32_t total_length;
	uint8_t is_final;  /* 1 = final (End-of-Body), 0 = not final (Body) */
	uint8_t name_len;  /* UTF-16BE with null terminator */
	uint8_t type_len;  /* MIME type string with null terminator */
	uint16_t body_len;
	uint8_t data[];    /* name[name_len] + type[type_len] + body[body_len] */
} __packed;

/* 0x16: Pull the server's default business card. No parameters. */
#define BTP_OPP_CLIENT_PULL_BCARD               0x16

/* 0x17: Abort the current operation. No parameters. */
#define BTP_OPP_CLIENT_ABORT                    0x17

/* 0x18: Autonomously push a 2 MB BMP file.
 * Result reported via BTP_OPP_EV_CLIENT_PUSH. No parameters.
 */
#define BTP_OPP_CLIENT_PUSH_2MB                 0x18

/* Server: 0x20 - 0x26 */

/* 0x20: Register RFCOMM transport and SDP record. No parameters. */
#define BTP_OPP_SERVER_REGISTER                 0x20

/* 0x21: Send OBEX CONNECT response. */
#define BTP_OPP_SERVER_CONNECT_RSP              0x21
struct btp_opp_server_connect_rsp_cmd {
	uint16_t mopl;
	uint8_t rsp_code;
} __packed;

/* 0x22: Send OBEX DISCONNECT response. */
#define BTP_OPP_SERVER_DISCONNECT_RSP           0x22
struct btp_opp_server_disconnect_rsp_cmd {
	uint8_t rsp_code;
} __packed;

/* 0x23: Send OBEX PUT response. */
#define BTP_OPP_SERVER_PUSH_RSP                 0x23
struct btp_opp_server_push_rsp_cmd {
	uint8_t rsp_code;
} __packed;

/* 0x24: Send OBEX GET response for business card pull. */
#define BTP_OPP_SERVER_PULL_BCARD_RSP           0x24
struct btp_opp_server_pull_bcard_rsp_cmd {
	uint8_t rsp_code;
	uint8_t is_final;  /* 1 = End-of-Body, 0 = Body */
	uint16_t body_len;
	uint8_t body[];
} __packed;

/* 0x25: Send OBEX ABORT response. */
#define BTP_OPP_SERVER_ABORT_RSP                0x25
struct btp_opp_server_abort_rsp_cmd {
	uint8_t rsp_code;
} __packed;

/* 0x26: Pre-configure the response code for the next incoming OBEX PUT request. */
#define BTP_OPP_SERVER_PREPARE_PUSH_RSP         0x26
struct btp_opp_server_prepare_push_rsp_cmd {
	uint8_t rsp_code;
} __packed;

/* 0x80: Reported after BTP_OPP_DISCOVER completes.
 * rfcomm_channel: RFCOMM channel from Protocol Descriptor List; 0 if not found.
 * formats[]:      bt_opp_format values from SupportedFormatsList attribute.
 */
#define BTP_OPP_EV_DISCOVERED                    0x80
struct btp_opp_discovered_ev {
	uint8_t rfcomm_channel;
	uint8_t formats_count;
	uint8_t formats[];
} __packed;

/* Client: 0x81 - 0x87 */

#define BTP_OPP_EV_CLIENT_TRANSPORT_CONNECTED    0x81  /* No payload. */
#define BTP_OPP_EV_CLIENT_TRANSPORT_DISCONNECTED 0x82  /* No payload. */

#define BTP_OPP_EV_CLIENT_CONNECTED              0x83
struct btp_opp_client_connected_ev {
	uint8_t rsp_code;
	uint8_t version;
	uint16_t mopl;
} __packed;

#define BTP_OPP_EV_CLIENT_DISCONNECTED           0x84
struct btp_opp_client_disconnected_ev {
	uint8_t rsp_code;
} __packed;

#define BTP_OPP_EV_CLIENT_PUSH                   0x85
struct btp_opp_client_push_ev {
	uint8_t rsp_code;
} __packed;

#define BTP_OPP_EV_CLIENT_PULL_BCARD             0x86
struct btp_opp_client_pull_bcard_ev {
	uint8_t rsp_code;
	uint16_t data_len;
	uint8_t data[];
} __packed;

#define BTP_OPP_EV_CLIENT_ABORT                  0x87
struct btp_opp_client_abort_ev {
	uint8_t rsp_code;
} __packed;

/* Server: 0x90 - 0x96 */

#define BTP_OPP_EV_SERVER_TRANSPORT_CONNECTED    0x90  /* No payload. */
#define BTP_OPP_EV_SERVER_TRANSPORT_DISCONNECTED 0x91  /* No payload. */

#define BTP_OPP_EV_SERVER_CONNECTED              0x92
struct btp_opp_server_connected_ev {
	uint8_t version;
	uint16_t mopl;
} __packed;

#define BTP_OPP_EV_SERVER_DISCONNECTED           0x93  /* No payload. */

#define BTP_OPP_EV_SERVER_PUSH                   0x94
struct btp_opp_server_push_ev {
	uint32_t total_length;
	uint8_t is_final;  /* 1 = End-of-Body (final), 0 = Body (more follows) */
	uint8_t name_len;
	uint8_t type_len;
	uint16_t body_len;
	uint8_t data[];    /* name[name_len] + type[type_len] + body[body_len] */
} __packed;

#define BTP_OPP_EV_SERVER_PULL_BCARD             0x95  /* No payload. */
#define BTP_OPP_EV_SERVER_ABORT                  0x96  /* No payload. */
