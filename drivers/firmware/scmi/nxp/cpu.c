/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/kernel.h>

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, nxp_scmi_cpu), NULL);

int scmi_cpu_sleep_mode_set(struct scmi_cpu_sleep_mode_config *cfg)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_CPU_DOMAIN);
	struct scmi_message msg, reply;
	int status, ret;
	bool use_polling;

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

	/* Set the PM-related scmi api to use poll mode to ensure that
	 * the CPU is not woken up by unnecessary scmi interrupts
	 */
	use_polling = true;

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_cpu_pd_lpm_set(struct scmi_cpu_pd_lpm_config *cfg)
{
	struct scmi_protocol *proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_CPU_DOMAIN);
	struct scmi_message msg, reply;
	int status, ret;
	bool use_polling;

	/* sanity checks */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CPU_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CPU_DOMAIN_MSG_CPU_PD_LPM_CONFIG_SET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	use_polling = true;

	ret = scmi_send_message(proto, &msg, &reply, use_polling);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}

int scmi_cpu_set_irq_mask(struct scmi_cpu_irq_mask_config *cfg)
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

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_CPU_DOMAIN_MSG_CPU_IRQ_WAKE_SET, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(*cfg);
	msg.content = cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, true);
	if (ret < 0) {
		return ret;
	}

	return scmi_status_to_errno(status);
}
