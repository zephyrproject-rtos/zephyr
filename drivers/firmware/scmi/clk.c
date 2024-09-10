/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/clk.h>
#include <string.h>

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

int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate)
{
	struct scmi_message msg, reply;
	int ret;
	struct scmi_clock_rate_set_reply reply_buffer;

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

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	*rate = reply_buffer.rate[0];

	return 0;
}

int scmi_clock_config_set(struct scmi_protocol *proto,
			  struct scmi_clock_config *cfg)
{
	struct scmi_message msg, reply;
	int status;

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

	return scmi_send_message(proto, &msg, &reply);
}

int scmi_clock_protocol_attributes(struct scmi_protocol *proto, uint32_t *attributes)
{
	struct scmi_message msg, reply;
	struct scmi_clock_attributes_reply reply_buffer;
	int ret;

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

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	*attributes = reply_buffer.attributes;

	return 0;
}
