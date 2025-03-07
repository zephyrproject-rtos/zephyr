/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, nxp_scmi_cpu), NULL);

int scmi_cpu_sleep_mode_set(struct scmi_cpu_sleep_mode_config *cfg)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_CPU_DOMAIN);
	struct scmi_message msg, reply;
	int status, ret;

	/* sanity checks */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CPU_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CPU_DOMAIN_MSG_CPU_SLEEP_MODE_SET, SCMI_COMMAND,
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
