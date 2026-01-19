/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/system.h>
#include <string.h>
#include <zephyr/kernel.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, arm_scmi_system), NULL,
		SCMI_SYSTEM_POWER_PROTOCOL_SUPPORTED_VERSION);

int scmi_system_protocol_version(uint32_t *version)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_SYSTEM);

	return scmi_protocol_get_version(proto, version);
}

int scmi_system_protocol_attributes(uint32_t *attributes)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_SYSTEM);

	return scmi_protocol_attributes_get(proto, attributes);
}

int scmi_system_protocol_message_attributes(uint32_t message_id, uint32_t *attributes)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_SYSTEM);

	return scmi_protocol_message_attributes_get(proto, message_id, attributes);
}

int scmi_system_protocol_version_negotiate(uint32_t version)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_SYSTEM);

	return scmi_protocol_version_negotiate(proto, version);
}

int scmi_system_power_state_set(struct scmi_system_power_state_config *cfg)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_SYSTEM);
	struct scmi_message msg, reply;
	int32_t status;
	int ret;
	bool use_polling;

	/* input validation */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_SYSTEM) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_SYSTEM_MSG_POWER_STATE_SET, SCMI_COMMAND,
					proto->id, 0x0);
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
