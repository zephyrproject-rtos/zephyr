/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/power.h>
#include <string.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, arm_scmi_power), NULL);

struct scmi_power_state_get_reply {
	int32_t status;
	uint32_t power_state;
};

int scmi_power_state_get(uint32_t domain_id, uint32_t *power_state)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_POWER_DOMAIN);
	struct scmi_power_state_get_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	/* sanity checks */
	if (!proto || !power_state) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_POWER_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_POWER_DOMAIN_MSG_POWER_STATE_GET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(reply_buffer);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	*power_state = reply_buffer.power_state;

	return 0;
}

int scmi_power_state_set(struct scmi_power_state_config *cfg)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_POWER_DOMAIN);
	struct scmi_message msg, reply;
	int status, ret;

	/* sanity checks */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_POWER_DOMAIN) {
		return -EINVAL;
	}

	/* Currently ASYNC flag is not supported. */
	if (cfg->flags & SCMI_POWER_STATE_SET_FLAGS_ASYNC) {
		return -ENOTSUP;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_POWER_DOMAIN_MSG_POWER_STATE_SET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	if (status != SCMI_SUCCESS) {
		return scmi_status_to_errno(status);
	}

	return 0;
}
