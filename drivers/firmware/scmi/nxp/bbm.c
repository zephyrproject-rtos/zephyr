/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/nxp/bbm.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(scmi_nxp_bbm);

static uint32_t scmi_nxp_bbm_events[] = {
	SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT,
};

static struct scmi_protocol_event bbm_event = {
	.evts = scmi_nxp_bbm_events,
	.num_events = ARRAY_SIZE(scmi_nxp_bbm_events),
	.cb = scmi_bbm_event_protocol_cb,
};

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, nxp_scmi_bbm), NULL, &bbm_event);

int scmi_bbm_button_notify(uint32_t flags)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_NXP_BBM);
	struct scmi_message msg, reply;
	int32_t status, ret;

	/* sanity checks */
	if (proto->id != SCMI_PROTOCOL_NXP_BBM) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_PROTO_BBM_BBM_BUTTON_NOTIFY, SCMI_COMMAND,
					proto->id, 0);
	msg.len = sizeof(flags);
	msg.content = &flags;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, k_is_pre_kernel());
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int scmi_bbm_button_event(uint32_t *flags)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_NXP_BBM);
	struct scmi_message msg;
	int32_t ret;

	/* sanity checks */
	if (proto->id != SCMI_PROTOCOL_NXP_BBM) {
		return -EINVAL;
	}

	/* TODO: Implement token handling for P2A flow to correctly associate notifications */
	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT, SCMI_NOTIFICATION,
			proto->id, 0x0);
	msg.len = sizeof(flags);
	msg.content = flags;

	ret = scmi_read_message(proto, &msg);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int scmi_bbm_event_protocol_cb(int32_t msg_id)
{
	uint32_t flags;
	int32_t ret = 0;

	if (msg_id == SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT) {
		ret = scmi_bbm_button_event(&flags);
		if (ret < 0) {
			LOG_ERR("failed to read bbm button event to shmem: %d", ret);
		} else {
			LOG_INF("SCMI BBM BUTTON notification: flags=0x%08X", flags);
		}
	}

	return ret;
}
