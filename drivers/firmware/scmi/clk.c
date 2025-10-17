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

int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate)
{
	struct scmi_message msg, reply;
	int ret;
	struct scmi_clock_rate_set_reply reply_buffer;
	bool use_polling;

	/* sanity checks */
	if (!proto || !rate) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_CLOCK_RATE_GET,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
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
	bool use_polling;

	/* sanity checks */
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

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_CLOCK_RATE_SET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
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
	bool use_polling;

	/* sanity checks */
	if (!proto || !parent_id) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr =
		SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_CLOCK_PARENT_GET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(clk_id);
	msg.content = &clk_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
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
	bool use_polling;

	/* sanity checks */
	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr =
		SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_CLOCK_PARENT_SET, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(cfg);
	msg.content = &cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
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
	bool use_polling;

	/* sanity checks */
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

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_CLOCK_CONFIG_SET,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_clock_protocol_attributes(struct scmi_protocol *proto, uint32_t *attributes)
{
	struct scmi_message msg, reply;
	struct scmi_clock_attributes_reply reply_buffer;
	int ret;
	bool use_polling;

	/* sanity checks */
	if (!proto || !attributes) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CLK_MSG_PROTOCOL_ATTRIBUTES,
					SCMI_COMMAND, proto->id, 0x0);
	/* command has no parameters */
	msg.len = 0x0;
	msg.content = NULL;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != 0) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*attributes = reply_buffer.attributes;

	return 0;
}
