/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/clk.h>
#include <string.h>
#include <zephyr/kernel.h>

/* TODO: if extended attributes are supported this should be moved
 * to the header file so that users will have access to it.
 */
#define SCMI_CLK_CONFIG_EA_MASK GENMASK(23, 16)

enum scmi_clock_message {
	CLOCK_ATTRIBUTES = 0x3,
	CLOCK_DESCRIBE_RATES = 0x4,
	CLOCK_RATE_SET = 0x5,
	CLOCK_RATE_GET = 0x6,
	CLOCK_CONFIG_SET = 0x7,
	CLOCK_NAME_GET = 0x8,
	CLOCK_RATE_NOTIFY = 0x9,
	CLOCK_RATE_CHANGE_REQUESTED_NOTIFY = 0xa,
	CLOCK_CONFIG_GET = 0xb,
	CLOCK_POSSIBLE_PARENTS_GET = 0xc,
	CLOCK_PARENT_SET = 0xd,
	CLOCK_PARENT_GET = 0xe,
	CLOCK_GET_PERMISSIONS = 0xf,
};

struct scmi_clock_attributes_reply {
	int32_t status;
	uint32_t attributes;
};

struct scmi_clock_rate_set_reply {
	int32_t status;
	uint32_t rate[2];
};

struct scmi_clock_parent_get_reply {
	int32_t status;
	uint32_t parent_id;
};

struct scmi_clock_parent_config {
	uint32_t clk_id;
	uint32_t parent_id;
};

struct scmi_clock_get_permissions_reply {
	int32_t status;
	uint32_t permissions;
};

struct scmi_clock_attributes_v2 {
	int32_t status;
	uint32_t attributes;
	uint8_t clock_name[SCMI_CLK_NAME_LEN];
} __packed;

struct scmi_clock_attributes_v3 {
	int32_t status;
	uint32_t attributes;
	uint8_t clock_name[SCMI_CLK_NAME_LEN];
	uint32_t clock_enable_delay;
} __packed;

int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate)
{
	struct scmi_message msg, reply;
	int ret;
	struct scmi_clock_rate_set_reply reply_buffer;

	/* input validation */
	if (!proto || !rate) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_RATE_GET,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*rate = reply_buffer.rate[0];

	return 0;
}

int scmi_clock_rate_set(struct scmi_protocol *proto, struct scmi_clock_rate_config *cfg)
{
	struct scmi_message msg, reply;
	int status, ret;

	/* input validation */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	/* Currently ASYNC flag is not supported. */
	if (cfg->flags & SCMI_CLK_RATE_SET_FLAGS_ASYNC) {
		return -ENOTSUP;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_RATE_SET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_clock_parent_get(struct scmi_protocol *proto, uint32_t clk_id, uint32_t *parent_id)
{
	struct scmi_message msg, reply;
	int ret;
	struct scmi_clock_parent_get_reply reply_buffer;

	/* input validation */
	if (!proto || !parent_id) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr =
		SCMI_MESSAGE_HDR_MAKE(CLOCK_PARENT_GET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*parent_id = reply_buffer.parent_id;

	return 0;
}

int scmi_clock_parent_set(struct scmi_protocol *proto, uint32_t clk_id, uint32_t parent_id)
{
	struct scmi_clock_parent_config cfg = {.clk_id = clk_id, .parent_id = parent_id};
	struct scmi_message msg, reply;
	int status, ret;

	/* input validation */
	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr =
		SCMI_MESSAGE_HDR_MAKE(CLOCK_PARENT_SET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(cfg);
	msg.content = &cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_clock_config_set(struct scmi_protocol *proto,
			  struct scmi_clock_config *cfg)
{
	struct scmi_message msg, reply;
	int status, ret;

	/* input validation */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	/* extended attributes currently not supported */
	if (cfg->attributes & SCMI_CLK_CONFIG_EA_MASK) {
		return -ENOTSUP;
	}

	/* invalid because extended attributes are not supported */
	if (SCMI_CLK_CONFIG_ENABLE_DISABLE(cfg->attributes) == 3) {
		return -ENOTSUP;
	}

	/* this is a reserved value */
	if (SCMI_CLK_CONFIG_ENABLE_DISABLE(cfg->attributes) == 2) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_CONFIG_SET,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

static int scmi_clock_attributes_v2(struct scmi_protocol *proto, uint32_t clk_id,
				    struct scmi_clock_attributes *attributes)
{
	struct scmi_clock_attributes_v2 v2;
	struct scmi_message msg, reply;
	int ret;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_ATTRIBUTES, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(v2);
	reply.content = &v2;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (v2.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(v2.status);
	}
	/* clock_enable_delay only available in protocol v3.0+ */
	attributes->clock_enable_delay = 0;
	attributes->attributes = v2.attributes;
	memcpy(attributes->clock_name, v2.clock_name, SCMI_CLK_NAME_LEN);

	return 0;
}

static int scmi_clock_attributes_v3(struct scmi_protocol *proto, uint32_t clk_id,
				    struct scmi_clock_attributes *attributes)
{
	struct scmi_clock_attributes_v3 v3;
	struct scmi_message msg, reply;
	int ret;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_ATTRIBUTES,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(v3);
	reply.content = &v3;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (v3.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(v3.status);
	}

	attributes->attributes = v3.attributes;
	attributes->clock_enable_delay = v3.clock_enable_delay;
	memcpy(attributes->clock_name, v3.clock_name, SCMI_CLK_NAME_LEN);

	return 0;
}

int scmi_clock_attributes(struct scmi_protocol *proto, uint32_t clk_id,
			  struct scmi_clock_attributes *attributes)
{
	if (!proto || !attributes) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	if (SCMI_PROTO_VER_MAJOR(proto->version) >= 3) {
		return scmi_clock_attributes_v3(proto, clk_id, attributes);
	} else {
		return scmi_clock_attributes_v2(proto, clk_id, attributes);
	}
}

int scmi_clock_get_permissions(struct scmi_protocol *proto, uint32_t clk_id,
			       uint32_t *permissions)
{
	struct scmi_clock_get_permissions_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !permissions) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(CLOCK_GET_PERMISSIONS,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status < 0) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*permissions = reply_buffer.permissions;

	return 0;
}
