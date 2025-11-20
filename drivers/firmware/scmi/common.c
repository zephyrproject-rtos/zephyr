/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI Common Protocol Commands Implementation
 *
 * This file contains the implementation of the SCMI commands that are
 * common to all protocols (generic or vendor-specific) as listed in
 * ARM SCMI Specification such as v4.0 (DEN0056F), Section 3.2.2 "Base protocol Commands".
 *
 * The following common commands are implemented:
 * - PROTOCOL_VERSION (0x0): Query protocol version
 * - PROTOCOL_ATTRIBUTES (0x1): Get protocol-specific attributes
 * - MESSAGE_ATTRIBUTES (0x2): Query message capabilities
 * - NEGOTIATE_PROTOCOL_VERSION (0x10): Negotiate protocol version support
 *
 * These commands provide standardized interfaces that can be reused across
 * different SCMI protocol implementations, ensuring consistency and reducing
 * code duplication.
 *
 * Reference: ARM System Control and Management Interface Platform Design Document
 * Version 4.0, Document number: DEN0056F
 * Available at: https://developer.arm.com/documentation/den0056/latest
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>

struct scmi_protocol_version_reply {
	int32_t status;
	uint32_t version;
};

struct scmi_protocol_attributes_reply {
	int32_t status;
	uint32_t attributes;
};

struct scmi_protocol_message_attributes_reply {
	int32_t status;
	uint32_t attributes;
};

int scmi_protocol_get_version(struct scmi_protocol *proto, uint32_t *version)
{
	struct scmi_protocol_version_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !version) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_MSG_PROTOCOL_VERSION, SCMI_COMMAND,
			proto->id, 0x0);
	msg.len = 0;
	msg.content = NULL;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, k_is_pre_kernel());
	if (ret < 0) {
		return ret;
	}

	*version = reply_buffer.version;

	return scmi_status_to_errno(reply_buffer.status);
}

int scmi_protocol_attributes_get(struct scmi_protocol *proto, uint32_t *attributes)
{
	struct scmi_protocol_attributes_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !attributes) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_MSG_PROTOCOL_ATTRIBUTES, SCMI_COMMAND,
			proto->id, 0x0);
	msg.len = 0;
	msg.content = NULL;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, k_is_pre_kernel());
	if (ret < 0) {
		return ret;
	}

	*attributes = reply_buffer.attributes;

	return scmi_status_to_errno(reply_buffer.status);
}

int scmi_protocol_message_attributes_get(struct scmi_protocol *proto,
					uint32_t message_id, uint32_t *attributes)
{
	struct scmi_protocol_message_attributes_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !attributes) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_MSG_MESSAGE_ATTRIBUTES, SCMI_COMMAND,
			proto->id, 0x0);
	msg.len = sizeof(message_id);
	msg.content = &message_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, true);
	if (ret < 0) {
		return ret;
	}

	*attributes = reply_buffer.attributes;

	return scmi_status_to_errno(reply_buffer.status);
}

int scmi_protocol_version_negotiate(struct scmi_protocol *proto, uint32_t version)
{
	struct scmi_message msg, reply;
	int32_t status;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_MSG_NEGOTIATE_PROTOCOL_VERSION, SCMI_COMMAND,
			proto->id, 0x0);
	msg.len = sizeof(version);
	msg.content = &version;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, k_is_pre_kernel());
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}
