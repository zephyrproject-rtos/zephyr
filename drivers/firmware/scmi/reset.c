/*
 * Copyright 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/reset.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(arm_scmi_reset);

/*
 * SCMI Reset protocol
 */

enum scmi_reset_protocol_cmd {
	PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	RESET_DOMAIN_ATTRIBUTES = 0x3,
	RESET = 0x4,
	RESET_NOTIFY = 0x5,
	RESET_DOMAIN_NAME_GET = 0x6,
};

/* PROTOCOL_ATTRIBUTES */
#define SCMI_RESET_ATTR_NUM_DOMAINS           GENMASK(15, 0)
#define SCMI_RESET_ATTR_GET_NUM_DOMAINS(attr) FIELD_GET(SCMI_RESET_ATTR_NUM_DOMAINS, (attr))

/* RESET_DOMAIN_ATTRIBUTES */
struct scmi_msg_reset_domain_attr_reply {
	int32_t status;
	uint32_t attr;
#define SCMI_RESET_ATTR_SUPPORTS_ASYNC     BIT(31)
#define SCMI_RESET_ATTR_SUPPORTS_NOTIFY    BIT(30)
#define SCMI_RESET_ATTR_SUPPORTS_EXT_NAMES BIT(29)
	uint32_t latency;
#define SCMI_RESET_ATTR_LATENCY_UNK 0xffffffff
	char name[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* RESET */
struct scmi_msg_reset_domain_reset_config {
	uint32_t domain_id;
	uint32_t flags;
#define SCMI_RESET_AUTONOMOUS         BIT(0)
#define SCMI_RESET_EXPLICIT_ASSERT    BIT(1)
#define SCMI_RESET_ASYNCHRONOUS_RESET BIT(2)
	uint32_t reset_state;
#define SCMI_RESET_ARCH_COLD_RESET 0
} __packed;

int scmi_reset_get_attr(struct scmi_protocol *proto, uint16_t *num_domains)
{
	uint32_t attributes = 0;
	int ret;

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	ret = scmi_protocol_attributes_get(proto, &attributes);
	if (ret < 0) {
		return ret;
	}

	*num_domains = SCMI_RESET_ATTR_GET_NUM_DOMAINS(attributes);

	return 0;
}

int scmi_reset_domain_get_attr(struct scmi_protocol *proto, uint32_t domain_id,
			       struct scmi_reset_domain_attr *dom_attr)
{
	struct scmi_msg_reset_domain_attr_reply reply_buffer;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !dom_attr) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(RESET_DOMAIN_ATTRIBUTES, SCMI_COMMAND, proto->id,
					0x0);
	msg.len = sizeof(domain_id);
	msg.content = &domain_id;

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

	LOG_DBG("scmi reset domain:%s get attributes attr:%x latency:%x", reply_buffer.name,
		reply_buffer.attr, reply_buffer.latency);

	strncpy(dom_attr->name, reply_buffer.name, sizeof(dom_attr->name));
	dom_attr->latency = reply_buffer.latency;
	dom_attr->is_latency_valid = true;
	if (reply_buffer.latency == SCMI_RESET_ATTR_LATENCY_UNK) {
		dom_attr->latency = 0;
		dom_attr->is_latency_valid = false;
	}
	dom_attr->is_async_sup = !!(reply_buffer.attr & SCMI_RESET_ATTR_SUPPORTS_ASYNC);
	dom_attr->is_notifications_sup = !!(reply_buffer.attr & SCMI_RESET_ATTR_SUPPORTS_NOTIFY);

	if (reply_buffer.attr & SCMI_RESET_ATTR_SUPPORTS_EXT_NAMES) {
		/* TODO: get long name by RESET_DOMAIN_NAME_GET */
	}

	return 0;
}

static int scmi_reset_domain(struct scmi_protocol *proto,
			     struct scmi_msg_reset_domain_reset_config *cfg)
{
	struct scmi_message msg, reply;
	int32_t status, ret;

	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(RESET, SCMI_COMMAND, proto->id, 0x0);
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

int scmi_reset_domain_assert(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_config cfg;
	int ret;

	cfg.domain_id = id;
	cfg.flags = SCMI_RESET_EXPLICIT_ASSERT;
	cfg.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	ret = scmi_reset_domain(proto, &cfg);
	if (ret < 0) {
		LOG_ERR("scmi reset:%u assert failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u assert", id);

	return ret;
}

int scmi_reset_domain_deassert(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_config cfg;
	int ret;

	cfg.domain_id = id;
	cfg.flags = 0;
	cfg.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	ret = scmi_reset_domain(proto, &cfg);
	if (ret < 0) {
		LOG_ERR("scmi reset:%d deassert failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u deassert", id);

	return ret;
}

int scmi_reset_domain_toggle(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_config cfg;
	int ret;

	cfg.domain_id = id;
	cfg.flags = SCMI_RESET_AUTONOMOUS;
	cfg.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	ret = scmi_reset_domain(proto, &cfg);
	if (ret < 0) {
		LOG_ERR("scmi reset:%d toggle failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u toggle", id);

	return ret;
}
