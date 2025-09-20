/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/nxp/bbm.h>
#include <zephyr/drivers/firmware/scmi/shmem.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, nxp_scmi_bbm), NULL);

LOG_MODULE_REGISTER(scmi_nxp_bbm);

extern uint32_t scmi_p2a_header_token;

static uint32_t scmi_nxp_bbm_events[] = {
	SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT,
};

int scmi_bbm_button_notify(uint32_t flags)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_NXP_BBM);
	struct scmi_message msg, reply;
	int32_t status, ret;
	bool use_polling;

	/* sanity checks */
	if (proto->id != SCMI_PROTOCOL_NXP_BBM) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_PROTO_BBM_BBM_BUTTON_NOTIFY, SCMI_COMMAND,
					proto->id, 0);
	msg.len = sizeof(uint32_t);
	msg.content = &flags;

	reply.hdr = msg.hdr;
	reply.len = sizeof(uint32_t);
	reply.content = &status;

	use_polling = k_is_pre_kernel();

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
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

static void scmi_bbm_event_protocol_cb(struct scmi_channel *channel, struct scmi_message msg)
{
	uint32_t flags;
	int32_t ret;
	uint32_t msg_id = SCMI_MESSAGE_HDR_EX_MSGID(msg.hdr);

	if (msg_id == SCMI_PROTO_BBM_PROTOCOL_BUTTON_EVENT) {
		ret = scmi_bbm_button_event(&flags);
		if (ret < 0) {
			LOG_ERR("failed to read bbm button event to shmem: %d", ret);
		} else {
			printf("SCMI BBM BUTTON notification: flags=0x%08X\n", flags);
		}
	}
}

static struct scmi_protocol_event bbm_event = {
	.id = SCMI_PROTOCOL_NXP_BBM,
	.evts = scmi_nxp_bbm_events,
	.num_events = ARRAY_SIZE(scmi_nxp_bbm_events),
	.cb = scmi_bbm_event_protocol_cb,
};

static int scmi_nxp_bbm_event_init(void)
{
	return scmi_register_protocol_event_handler(&bbm_event);
}

SYS_INIT(scmi_nxp_bbm_event_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
