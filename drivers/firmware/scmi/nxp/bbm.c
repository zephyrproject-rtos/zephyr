/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/nxp/bbm.h>
#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <string.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, nxp_scmi_bbm), NULL);

extern uint32_t scmi_p2a_header_token;

int scmi_bbm_button_notify(uint32_t flags)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BBM);
	struct scmi_message msg, reply;
	int32_t status, ret;

	/* sanity checks */
	if (proto->id != SCMI_PROTOCOL_BBM) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_PROTO_BBM_BBM_BUTTON_NOTIFY, SCMI_COMMAND,
					proto->id, 0);
	msg.len = sizeof(uint32_t);
	msg.content = &flags;

	reply.hdr = msg.hdr;
	reply.len = sizeof(uint32_t);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int scmi_bbm_button_event(uint32_t *flags)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BBM);
	struct scmi_message msg;
	int32_t ret;

	/* sanity checks */
	if (proto->id != SCMI_PROTOCOL_BBM) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT, SCMI_NOTIFICATION,
			proto->id, scmi_p2a_header_token++);
	msg.len = sizeof(uint32_t);
	msg.content = flags;

	ret = scmi_read_message(proto, &msg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
