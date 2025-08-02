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
LOG_MODULE_REGISTER(pldm_host);

/* PLDM MCTP Message Type */
#define PLDM_MCTP_MESSAGE_TYPE 1

/* Local MCTP Endpoint ID that responds to requests */
#define LOCAL_EID 20
/* Local PLDM Terminus ID that responds to requests */
#define LOCAL_TID 1

/* Remote MCTP Endpoint ID that we respond to */
#define REMOTE_EID 10

#define MCTP_INTEGRITY_CHECK   0x80
#define MCTP_MESSAGE_TYPE_MASK 0x7F

K_SEM_DEFINE(mctp_rx, 0, 1);

const char *MESSAGE_TYPE_TO_STRING[] = {"Response", "Request", "Reserved", "Async Request Notify"};

const char *COMMAND_TO_STRING[] = {"UNDEFINED",      "SetTID",          "GetTID",
				   "GetPLDMVersion", "GetPLDMCommands", "SelectPLDMVersion"};

static struct mctp *mctp_ctx;

struct pldm_type_info {
	ver32_t version;
	uint8_t commands[32];
};

struct pldm_tid_info {
	uint8_t tid;
	uint8_t types[8]; /* PLDM Types supported, up to 256 types are representable */
	struct pldm_type_info
		*type_infos; /* An array to be allocated once the types are known... */
};

/* Response message buffer */
uint8_t mctp_msg[256];

/**
 * response data
 */
static uint8_t comp_code;
static uint8_t tid;
static uint8_t types[8];
static uint8_t commands[32];
static ver32_t version;

/* Discovery if and what a MCTP endpoint can do */
/* TODO pldm_discovery(struct mctp *mctp_ctx, uint8_t eid, struct pldm_tid_info *tid_info); */

static void pldm_rx_handler(uint8_t src_eid, void *data, void *msg, size_t msg_len)
{
	struct pldm_header_info hdr_info;
	const char *msg_type_str = "";
	const char *command_str = "";
	int rc;

	rc = unpack_pldm_header(msg, &hdr_info);
	if (rc != 0) {
		LOG_ERR("Failed unpacking pldm header");
	}

	if (hdr_info.msg_type < ARRAY_SIZE(MESSAGE_TYPE_TO_STRING)) {
		msg_type_str = MESSAGE_TYPE_TO_STRING[hdr_info.msg_type];
	}

	if (hdr_info.command < ARRAY_SIZE(COMMAND_TO_STRING)) {
		command_str = COMMAND_TO_STRING[hdr_info.command];
	}

	LOG_INF("received pldm message from mctp endpoint %d len %zu message type %d (%s) command "
		"%d (%s)",
		src_eid, msg_len, hdr_info.msg_type, msg_type_str, hdr_info.command, command_str);

	/* Handle the GetTID Response */
	if (hdr_info.command == PLDM_GET_TID && hdr_info.msg_type == PLDM_RESPONSE) {
		decode_get_tid_resp(msg, msg_len - sizeof(struct pldm_msg_hdr), &comp_code, &tid);
		LOG_INF("get tid response, completion code %d, tid %d", comp_code, tid);
	} else if (hdr_info.command == PLDM_GET_PLDM_TYPES && hdr_info.msg_type == PLDM_RESPONSE) {
		decode_get_types_resp(msg, msg_len - sizeof(struct pldm_msg_hdr), &comp_code,
				      (bitfield8_t *)types);
		LOG_INF("get types response, completion code %d", comp_code);
	} else if (hdr_info.command == PLDM_GET_PLDM_COMMANDS &&
		   hdr_info.msg_type == PLDM_RESPONSE) {
		decode_get_commands_resp(msg, msg_len - sizeof(struct pldm_msg_hdr), &comp_code,
					 (bitfield8_t *)types);
		LOG_INF("get commands response, completion code %d", comp_code);
	} else if (hdr_info.command == PLDM_GET_PLDM_VERSION &&
		   hdr_info.msg_type == PLDM_RESPONSE) {
		/* ignored, we only accept the first version response... */
		uint32_t next_transfer_handle;
		/* ignored */
		uint8_t transfer_flag;

		decode_get_version_resp(msg, msg_len - sizeof(struct pldm_msg_hdr), &comp_code,
			&next_transfer_handle, &transfer_flag, &version);

		LOG_INF("get version response, completion code %d, version %d.%d", comp_code,
			version.major, version.minor);
	} else {
		LOG_WRN("unhandled message command %d and type %d", hdr_info.command,
			hdr_info.msg_type);
	}
}

static void rx_message(uint8_t src_eid, bool tag_owner, uint8_t msg_tag, void *data, void *msg,
		       size_t len)
{
	LOG_INF("received message from mctp endpoint %d, msg_tag %d, len %zu", src_eid, msg_tag,
		len);
	LOG_HEXDUMP_INF(msg, len, "mctp rx message");

	if (len < 1) {
		LOG_ERR("MCTP Message should contain a message type and integrity check"
			" byte!");
		return;
	}

	/* Treat data as a buffer for byte wise access */
	uint8_t *msg_buf = msg;

	/* if the message endpoint id matches our local endpoint id, and its a pldm message, call
	 * the pldm_rx_message call
	 */
	if ((msg_buf[0] & MCTP_MESSAGE_TYPE_MASK) == PLDM_MCTP_MESSAGE_TYPE) {
		pldm_rx_handler(src_eid, data, &msg_buf[1], len - 1);
	}

	k_sem_give(&mctp_rx);
}

MCTP_UART_DT_DEFINE(mctp_host, DEVICE_DT_GET(DT_NODELABEL(arduino_serial)));

int main(void)
{
	int rc;

	LOG_INF("PLDM Host EID:%d on %s\n", LOCAL_EID, CONFIG_BOARD_TARGET);

	mctp_set_alloc_ops(malloc, free, realloc);
	mctp_ctx = mctp_init();
	__ASSERT_NO_MSG(mctp_ctx != NULL);
	mctp_register_bus(mctp_ctx, &mctp_host.binding, LOCAL_EID);
	mctp_set_rx_all(mctp_ctx, rx_message, NULL);
	mctp_uart_start_rx(&mctp_host);

	/* We can set this one time here */
	mctp_msg[0] = PLDM_MCTP_MESSAGE_TYPE;

	/* PLDM message is after the MCTP message type byte */
	struct pldm_msg *msg = &mctp_msg[1];
	uint32_t instance = 0;

	/* PLDM poll (discovery) loop, send a sequence of commands and wait on responses */
	while (true) {
		k_msleep(1000);
		rc = 0;

		/* GetTID request/response */
		uint8_t get_tid_request_size = PLDM_MSG_SIZE(0) + 1;

		encode_get_tid_req(instance, msg);
		instance++;

		LOG_HEXDUMP_INF(mctp_msg, get_tid_request_size, "pldm get_tid_request");
		rc = mctp_message_tx(mctp_ctx, REMOTE_EID, false, 0, mctp_msg,
				     get_tid_request_size);
		if (rc != 0) {
			LOG_WRN("Failed to send message, errno %d\n", rc);
			continue;
		} else {
			k_sem_take(&mctp_rx, K_MSEC(1000));
		}
		uint8_t get_types_request_size = PLDM_MSG_SIZE(0) + 1;

		/* GetPLDMTypes request/response */
		encode_get_types_req(instance, msg);
		instance++;

		LOG_HEXDUMP_INF(mctp_msg, get_types_request_size, "pldm get_types_request");
		rc = mctp_message_tx(mctp_ctx, REMOTE_EID, false, 0, mctp_msg,
				     get_types_request_size);
		if (rc != 0) {
			LOG_WRN("Failed to send message, errno %d\n", rc);
			continue;
		} else {
			k_sem_take(&mctp_rx, K_MSEC(1000));
		}

		/* TODO in the discovery case we'd iterate types to gather information on versions
		 * and available commands for each version (or a selected version)
		 */

		uint8_t pldm_type = PLDM_BASE;

		/* GetPLDMVersion request/response */
		uint32_t transfer_handle = 0;
		uint8_t transfer_opflag = PLDM_GET_FIRSTPART;
		uint8_t get_version_request_size = PLDM_MSG_SIZE(PLDM_GET_VERSION_REQ_BYTES) + 1;

		encode_get_version_req(instance, transfer_handle, transfer_opflag, pldm_type, msg);
		instance++;

		LOG_HEXDUMP_INF(mctp_msg, get_version_request_size, "pldm get_version_request");
		rc = mctp_message_tx(mctp_ctx, REMOTE_EID, false, 0, mctp_msg,
				     get_version_request_size);
		if (rc != 0) {
			LOG_WRN("Failed to send message, errno %d\n", rc);
			continue;
		} else {
			k_sem_take(&mctp_rx, K_MSEC(1000));
		}

		/* GetPLDMCommands request/response */
		uint8_t get_commands_request_size = PLDM_MSG_SIZE(PLDM_GET_COMMANDS_REQ_BYTES) + 1;

		encode_get_commands_req(instance, pldm_type, version, msg);
		instance++;

		LOG_HEXDUMP_INF(mctp_msg, get_version_request_size, "pldm get_commands_request");
		rc = mctp_message_tx(mctp_ctx, REMOTE_EID, false, 0, mctp_msg,
				     get_commands_request_size);
		if (rc != 0) {
			LOG_WRN("Failed to send message, errno %d\n", rc);
			continue;
		} else {
			k_sem_take(&mctp_rx, K_MSEC(1000));
		}
	}

	return 0;
}
