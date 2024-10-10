/*
 * Copyright 2024 EPAM Systems
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
#define SCMI_PROTOCOL_RESET_REV_MAJOR 0x1

enum scmi_reset_protocol_cmd {
	SCMI_RESET_MSG_PROTOCOL_VERSION = 0x0,
	SCMI_RESET_MSG_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_RESET_MSG_PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_RESET_MSG_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_RESET_MSG = 0x4,
	SCMI_RESET_MSG_NOTIFY = 0x5,
	SCMI_RESET_MSG_DOMAIN_NAME_GET = 0x6,
};

/* Reset PROTOCOL_ATTRIBUTES */
struct scmi_msg_reset_attributes_p2a {
	uint32_t attributes;
#define SCMI_RESET_ATTR_NUM_DOMAINS           GENMASK(15, 0)
#define SCMI_RESET_ATTR_GET_NUM_DOMAINS(attr) FIELD_GET(SCMI_RESET_ATTR_NUM_DOMAINS, (attr))
} __packed;

/* RESET_DOMAIN_ATTRIBUTES */
struct scmi_msg_reset_domain_attr_a2p {
	uint32_t domain_id;
} __packed;

/* RESET_DOMAIN_ATTRIBUTES */
struct scmi_msg_reset_domain_attr_p2a {
	uint32_t attr;
#define SCMI_RESET_ATTR_SUPPORTS_ASYNC     BIT(31)
#define SCMI_RESET_ATTR_SUPPORTS_NOTIFY    BIT(30)
#define SCMI_RESET_ATTR_SUPPORTS_EXT_NAMES BIT(29)
	uint32_t latency;
#define SCMI_RESET_ATTR_LATENCY_UNK1 0x7fffffff
#define SCMI_RESET_ATTR_LATENCY_UNK2 0xffffffff
	char name[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* RESET */
struct scmi_msg_reset_domain_reset_a2p {
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
	struct scmi_msg_reset_attributes_p2a attr;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !num_domains) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_RESET_MSG_PROTOCOL_ATTRIBUTES,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = 0;
	msg.content = NULL;

	reply.len = sizeof(attr);
	reply.content = &attr;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	*num_domains = SCMI_RESET_ATTR_GET_NUM_DOMAINS(attr.attributes);

	return 0;
}

int scmi_reset_domain_get_attr(struct scmi_protocol *proto, uint32_t id,
			       struct scmi_reset_domain_attr *dom_attr)
{
	struct scmi_msg_reset_domain_attr_a2p tx;
	struct scmi_msg_reset_domain_attr_p2a rx;
	struct scmi_message msg, reply;
	int ret;

	if (!proto || !dom_attr) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	tx.domain_id = id;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_RESET_MSG_DOMAIN_ATTRIBUTES, SCMI_COMMAND, proto->id,
					0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = sizeof(rx);
	reply.content = &rx;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("scmi reset domain:%s get attributes attr:%x latency:%x", rx.name, rx.attr,
		rx.latency);

	strncpy(dom_attr->name, rx.name, sizeof(dom_attr->name));
	dom_attr->latency = 0;
	dom_attr->is_latency_valid = false;
	if (rx.latency != SCMI_RESET_ATTR_LATENCY_UNK1 &&
	    rx.latency == SCMI_RESET_ATTR_LATENCY_UNK1) {
		dom_attr->latency = rx.latency;
		dom_attr->is_latency_valid = true;
	}
	dom_attr->is_async_sup = !!(rx.attr & SCMI_RESET_ATTR_SUPPORTS_ASYNC);
	dom_attr->is_notifications_sup = !!(rx.attr & SCMI_RESET_ATTR_SUPPORTS_NOTIFY);

	if (rx.attr & SCMI_RESET_ATTR_SUPPORTS_EXT_NAMES) {
		/* TODO: get long name */
	}

	return 0;
}

int scmi_reset_domain_assert(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_a2p tx;
	struct scmi_message msg, reply;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	tx.domain_id = id;
	tx.flags = SCMI_RESET_EXPLICIT_ASSERT;
	tx.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_RESET_MSG, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = 0;
	reply.content = NULL;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("scmi reset:%u assert failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u assert", id);

	return ret;
}

int scmi_reset_domain_deassert(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_a2p tx;
	struct scmi_message msg, reply;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	tx.domain_id = id;
	tx.flags = 0;
	tx.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_RESET_MSG, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = 0;
	reply.content = NULL;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("scmi reset:%d deassert failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u deassert", id);

	return ret;
}

int scmi_reset_domain_toggle(struct scmi_protocol *proto, uint32_t id)
{
	struct scmi_msg_reset_domain_reset_a2p tx;
	struct scmi_message msg, reply;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_RESET_DOMAIN) {
		return -EINVAL;
	}

	tx.domain_id = id;
	tx.flags = SCMI_RESET_AUTONOMOUS;
	tx.reset_state = SCMI_RESET_ARCH_COLD_RESET;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_RESET_MSG, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = 0;
	reply.content = NULL;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("scmi reset:%d toggle failed (%d)", id, ret);
	}

	LOG_DBG("scmi reset:%u toggle", id);

	return ret;
}
