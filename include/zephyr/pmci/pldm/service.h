/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_PLDM_SERVICE_H_
#define ZEPHYR_PLDM_SERVICE_H_

#include <stdint.h>
#include <libpldm/base.h>

const char *MESSAGE_TYPE_TO_STRING[] = {"Response", "Request", "Reserved", "Async Request Notify"};

const char *COMMAND_TO_STRING[] = {"UNDEFINED",      "SetTID",          "GetTID",
				   "GetPLDMVersion", "GetPLDMCommands", "SelectPLDMVersion"};

/**
 * @brief Set of functions that are expected to be implemented by PLDM base specification

 * Some of these functions are *mandatory* to comply with the PLDM specification, and common
 implementations
 * of these can use the common pldm_context struct to provide them.
 */
struct pldm_base_api {
	void (*get_tid_request)(uint8_t mctp_src_eid, struct pldm_header_info *hdr);
	void (*get_tid_response)(uint8_t mctp_src_eid, struct pldm_header_info *hdr,
				 uint8_t completion_code, uint8_t tid);
	void (*get_version_request)(uint8_t mctp_src_eid, struct pldm_header_info *hdr,
				    uint32_t data_transfer_handle, uint8_t flags,
				    uint8_t pldm_type);
	void (*get_version_response)(uint8_t mctp_src_eid, struct pldm_header_info *hdr,
				     uint8_t completion_code, uint32_t version);
	void (*get_types)(uint8_t mctp_src_eid, struct pldm_header_info *hdr);
	void (*get_commands)(uint8_t mctp_src_eid, struct pldm_header_info *hdr);
};

/**
 * @brief PLDM handler handles PLDM requests/response messages
 *
 * Struct should be embedded into a containing struct which holds all the needed contextual
 * information.
 */
struct pldm_handler {
	const struct pldm_base_api *api;
};

/**
 * @brief Handle a pldm message from mctp by routing to the appropriate function pointer
 */
void pldm_service_mctp_receive(struct pldm_handler *hndlr, uint8_t mctp_src_eid, uint8_t *msg, size_t msg_len);

/**
 * @brief Common base API implementations
 */
extern const struct pldm_base_api COMMON_BASE_API;

struct pldm_common_handler {
	struct pldm_handler handler;
	uint8_t tid;
	uint32_t base_version;
	uint8_t *commands;
	size_t num_commands;
	uint8_t *types;
	size_t num_types;
};

#define PLDM_COMMON_HANDLER_INIT(_tid, _base_version, _commands, _types)                           \
	{                                                                                          \
		.handler =                                                                         \
			{                                                                          \
				.api = COMMON_BASE_API,                                            \
			},                                                                         \
		.tid = _tid,                                                                       \
		.version = _base_version,                                                          \
		.commands = _commands,                                                             \
		.num_commands = ARRAY_SIZE(_commands).types = _types,                              \
		.num_types = ARRAY_SIZE(_types),                                                   \
	}

#endif /* ZEPHYR_PLDM_SERVICE_H_ */
