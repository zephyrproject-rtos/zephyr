/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <libpldm/base.h>
#include <libmctp.h>
#include <zephyr/pmci/mctp/mctp_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pldm_endpoint);

/* PLDM MCTP Message Type */
#define PLDM_MCTP_MESSAGE_TYPE 1

/* Local MCTP Endpoint ID that responds to requests */
#define LOCAL_EID 10
/* Local PLDM Terminus ID that responds to requests */
#define LOCAL_TID 2

/* Remote MCTP Endpoint ID that we request from to */
#define REMOTE_EID 20

#define MCTP_INTEGRITY_CHECK   0x80
#define MCTP_MESSAGE_TYPE_MASK 0x7F

const char *MESSAGE_TYPE_TO_STRING[] = {"Response", "Request", "Reserved", "Async Request Notify"};

const char *COMMAND_TO_STRING[] = {"UNDEFINED",      "SetTID",          "GetTID",
				   "GetPLDMVersion", "GetPLDMCommands", "SelectPLDMVersion"};

static struct mctp *mctp_ctx;

static void pldm_rx_handler(uint8_t src_eid, void *data, struct pldm_msg_hdr *msg_hdr, void *msg,
			    size_t msg_len)
{
	struct pldm_header_info hdr_info;
	const char *message_type_str = "";
	const char *command_str = "";
	int rc;

	rc = unpack_pldm_header(msg_hdr, &hdr_info);
	if (rc != 0) {
		LOG_ERR("Failed unpacking pldm header");
	}

	if (hdr_info.msg_type < ARRAY_SIZE(MESSAGE_TYPE_TO_STRING)) {
		message_type_str = MESSAGE_TYPE_TO_STRING[hdr_info.msg_type];
	}

	if (hdr_info.command < ARRAY_SIZE(COMMAND_TO_STRING)) {
		command_str = COMMAND_TO_STRING[hdr_info.command];
	}

	LOG_INF("received pldm message from mctp endpoint %d len %zu message type %d (%s) command "
		"%d (%s)",
		src_eid, msg_len, hdr_info.msg_type, message_type_str, hdr_info.command,
		command_str);

	/* Handle the GetTID command */
	if (hdr_info.command == PLDM_GET_TID && hdr_info.msg_type == PLDM_REQUEST) {
		/* Response buffer for the GetTID command needs
		 * pldm response header (4 bytes) + 1 bytes (the tid is 1 byte) + 1 byte (mctp
		 * message type byte)
		 */
		uint8_t resp_msg_buf[PLDM_MSG_SIZE(sizeof(struct pldm_get_tid_resp)) + 1];

		resp_msg_buf[0] = PLDM_MCTP_MESSAGE_TYPE;

		rc = encode_get_tid_resp(hdr_info.instance, PLDM_SUCCESS, LOCAL_TID,
				    (struct pldm_msg *)&resp_msg_buf[1]);
		__ASSERT(rc == PLDM_SUCCESS, "Encoding pldm response should succeed");

		rc = mctp_message_tx(mctp_ctx, src_eid, false, 0, resp_msg_buf,
				     sizeof(resp_msg_buf));
		__ASSERT(rc == 0, "Sending response to GetTID should succeed");
	} else if (hdr_info.command == PLDM_GET_PLDM_TYPES && hdr_info.msg_type == PLDM_REQUEST) {
		/* Response buffer for the GetTID command needs
		 * pldm response header (4 bytes) + 1 bytes (the tid is 1 byte) + 1 byte (mctp
		 * message type byte)
		 */
		uint8_t resp_msg_buf[PLDM_MSG_SIZE(PLDM_GET_TYPES_RESP_BYTES) + 1];
		uint8_t types[8];
		
		resp_msg_buf[0] = PLDM_MCTP_MESSAGE_TYPE;

		types[0] = PLDM_BASE; 
		rc = encode_get_types_resp(hdr_info.instance, PLDM_SUCCESS, (const bitfield8_t *)types,
				    (struct pldm_msg *)&resp_msg_buf[1]);
		__ASSERT(rc == PLDM_SUCCESS, "Encoding pldm response should succeed");

		rc = mctp_message_tx(mctp_ctx, src_eid, false, 0, resp_msg_buf,
				     sizeof(resp_msg_buf));
		__ASSERT(rc == 0, "Sending response to GetTypes should succeed");	} else if (hdr_info.command == PLDM_GET_PLDM_COMMANDS && hdr_info.msg_type == PLDM_REQUEST) {
		
	} else if (hdr_info.command == PLDM_GET_PLDM_VERSION && hdr_info.msg_type == PLDM_REQUEST) {
		uint32_t transfer_handle;
		uint8_t transfer_opflag;
		uint8_t type;

		rc = decode_get_version_req(msg, PLDM_GET_VERSION_REQ_BYTES, &transfer_handle, &transfer_opflag, &type);
		__ASSERT(rc == PLDM_SUCCESS, "Decoding GetVersion request should succeed");

		
		/* Response buffer for the GetTID command needs
		 * pldm response header (4 bytes) + 1 bytes (the tid is 1 byte) + 1 byte (mctp
		 * message type byte)
		 */
		uint8_t resp_msg_buf[PLDM_MSG_SIZE(PLDM_GET_VERSION_RESP_BYTES) + 1];
		ver32_t version = { .major = 1 };
		
		resp_msg_buf[0] = PLDM_MCTP_MESSAGE_TYPE;

		rc = encode_get_version_resp(hdr_info.instance, PLDM_SUCCESS, 0, 0, &version, sizeof(ver32_t),
				    (struct pldm_msg *)&resp_msg_buf[1]);
		__ASSERT(rc == PLDM_SUCCESS, "Encoding pldm response should succeed");

		rc = mctp_message_tx(mctp_ctx, src_eid, false, 0, resp_msg_buf,
				     sizeof(resp_msg_buf));
		__ASSERT(rc == 0, "Sending response to GetVersion should succeed");	} else if (hdr_info.command == PLDM_GET_PLDM_COMMANDS && hdr_info.msg_type == PLDM_REQUEST) {
	} else if (hdr_info.command == PLDM_GET_PLDM_COMMANDS && hdr_info.msg_type == PLDM_REQUEST) {
		uint8_t type;
		ver32_t vers;

		rc = decode_get_commands_req(msg, PLDM_GET_COMMANDS_REQ_BYTES, &type, &vers);

		__ASSERT(rc == PLDM_SUCCESS, "Decoding GetCommands request should succeed");

		uint8_t resp_msg_buf[PLDM_MSG_SIZE(PLDM_GET_COMMANDS_RESP_BYTES) + 1];
		uint8_t commands[32];

		resp_msg_buf[0] = PLDM_MCTP_MESSAGE_TYPE;
		commands[0] =  PLDM_GET_TID | PLDM_GET_PLDM_VERSION | PLDM_GET_PLDM_TYPES | PLDM_GET_PLDM_COMMANDS;
		rc = encode_get_commands_resp(hdr_info.instance, PLDM_SUCCESS, (const bitfield8_t *)commands,
				    (struct pldm_msg *)&resp_msg_buf[1]);
		__ASSERT(rc == PLDM_SUCCESS, "Encoding pldm response should succeed");

		rc = mctp_message_tx(mctp_ctx, src_eid, false, 0, resp_msg_buf,
				     sizeof(resp_msg_buf));
		__ASSERT(rc == 0, "Sending response to GetCommands should succeed");	} else if (hdr_info.command == PLDM_GET_PLDM_COMMANDS && hdr_info.msg_type == PLDM_REQUEST) {
	} else {
		LOG_WRN("Unhandled pldm message, command %d, type %d", hdr_info.command, hdr_info.msg_type);
	}
}

static void rx_message(uint8_t src_eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message from mctp endpoint %d, msg_tag %d, len %zu", src_eid, msg_tag,
		len);
	LOG_HEXDUMP_INF(msg, len, "mctp rx message");
	if (len < 1) {
		LOG_ERR("MCTP Message should contain a message type and integrity check byte!");
		return;
	}

	/* Treat data as a buffer for byte wise access */
	uint8_t *msg_buf = msg;

	/* If the message endpoint ID matches our local endpoint ID, and its a pldm message, call
	 * the pldm_rx_message call
	 */
	if ((msg_buf[0] & MCTP_MESSAGE_TYPE_MASK) == PLDM_MCTP_MESSAGE_TYPE) {
		/* HAZARD This is potentially error prone but libpldm provides little help here */
		struct pldm_msg_hdr *pldm_hdr = (struct pldm_msg_hdr *)&(msg_buf[1]);
		size_t pldm_msg_body_len = len - (1 + sizeof(struct pldm_msg_hdr));
		void *pldm_msg_body = &msg_buf[1 + sizeof(struct pldm_msg_hdr)];

		pldm_rx_handler(src_eid, msg, pldm_hdr, pldm_msg_body, pldm_msg_body_len);
	}
}

MCTP_UART_DT_DEFINE(mctp_host, DEVICE_DT_GET(DT_NODELABEL(arduino_serial)));

int main(void)
{
	LOG_INF("PLDM Endpoint EID:%d TID:%d on %s\n", LOCAL_EID, LOCAL_TID, CONFIG_BOARD_TARGET);

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_host.binding, LOCAL_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);
	mctp_uart_start_rx(&mctp_host);

	return 0;
}
