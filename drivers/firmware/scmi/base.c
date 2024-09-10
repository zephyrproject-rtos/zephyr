/*
 * System Control and Management Interface (SCMI) Base Protocol
 *
 * Copyright (c) 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arm_scmi_proto_base);

#include <zephyr/drivers/firmware/scmi/base.h>

#ifdef CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS
extern struct scmi_channel SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0);
#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */

STRUCT_SECTION_ITERABLE(scmi_protocol, SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE)) = {
	.id = SCMI_PROTOCOL_BASE,
#ifdef CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS
	.tx = &SCMI_TRANSPORT_CHAN_NAME(SCMI_PROTOCOL_BASE, 0),
#endif /* CONFIG_ARM_SCMI_TRANSPORT_HAS_STATIC_CHANNELS */
	.data = NULL,
};

/*
 * SCMI Base protocol
 */
enum scmi_base_protocol_cmd {
	SCMI_BASE_PROTOCOL_VERSION = 0x0,
	SCMI_BASE_PROTOCOL_ATTRIBUTES = 0x1,
	SCMI_BASE_PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	SCMI_BASE_DISCOVER_VENDOR = 0x3,
	SCMI_BASE_DISCOVER_SUB_VENDOR = 0x4,
	SCMI_BASE_DISCOVER_IMPLEMENT_VERSION = 0x5,
	SCMI_BASE_DISCOVER_LIST_PROTOCOLS = 0x6,
	SCMI_BASE_DISCOVER_AGENT = 0x7,
	SCMI_BASE_NOTIFY_ERRORS = 0x8,
	SCMI_BASE_SET_DEVICE_PERMISSIONS = 0x9,
	SCMI_BASE_SET_PROTOCOL_PERMISSIONS = 0xa,
	SCMI_BASE_RESET_AGENT_CONFIGURATION = 0xb,
};

/*
 * SCMI Base protocol version info
 */

/* BASE PROTOCOL_ATTRIBUTES */
struct scmi_msg_base_attributes_p2a {
	uint8_t num_protocols;
	uint8_t num_agents;
	uint16_t reserved;
} __packed;

/* BASE_DISCOVER_VENDOR */
struct scmi_msg_base_vendor_id_p2a {
	char vendor_id[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* BASE_DISCOVER_SUB_VENDOR */
struct scmi_msg_base_subvendor_id_p2a {
	char subvendor_id[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* BASE_DISCOVER_IMPLEMENTATION_VERSION */
struct scmi_msg_base_impl_ver_p2a {
	uint32_t impl_ver;
} __packed;

/*
 * BASE_DISCOVER_AGENT
 */
struct scmi_msg_base_discover_agent_a2p {
	uint32_t agent_id;
} __packed;

struct scmi_msg_base_discover_agent_p2a {
	uint32_t agent_id;
	char name[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/*
 * BASE_SET_DEVICE_PERMISSIONS
 */
#define SCMI_BASE_DEVICE_ACCESS_ALLOW			BIT(0)

struct scmi_msg_base_set_device_permissions_a2p {
	uint32_t agent_id;
	uint32_t device_id;
	uint32_t flags;
} __packed;

/*
 * BASE_RESET_AGENT_CONFIGURATION
 */
#define SCMI_BASE_AGENT_PERMISSIONS_RESET		BIT(0)

struct scmi_msg_base_reset_agent_cfg_a2p {
	uint32_t agent_id;
	uint32_t flags;
} __packed;

static int scmi_base_get_version(struct scmi_protocol_version *version)
{
	struct scmi_protocol *proto;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	return scmi_core_get_version(proto, version);
}

#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)

static int scmi_base_xfer_no_tx(uint8_t msg_id, void *rx_buf, size_t rx_len)
{
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(msg_id,
					SCMI_COMMAND, proto->id, 0x0);
	msg.len = 0;
	msg.content = NULL;

	reply.len = rx_len;
	reply.content = rx_buf;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("base xfer failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int scmi_base_attributes_get(struct scmi_msg_base_attributes_p2a *attr)
{
	int ret;

	ret = scmi_base_xfer_no_tx(SCMI_BASE_PROTOCOL_ATTRIBUTES, attr, sizeof(*attr));
	if (ret) {
		LOG_ERR("base get attributes failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base attr num_protocols:0x%02x num_agents:0x%02x", attr->num_protocols,
		attr->num_agents);

	return 0;
}

static int scmi_base_vendor_id_get(struct scmi_msg_base_vendor_id_p2a *id)
{
	int ret;

	ret = scmi_base_xfer_no_tx(SCMI_BASE_DISCOVER_VENDOR, id, sizeof(*id));
	if (ret) {
		LOG_ERR("base get vendor id failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base vendor id:%s", id->vendor_id);

	return 0;
}

static int scmi_base_subvendor_id_get(struct scmi_msg_base_subvendor_id_p2a *id)
{
	int ret;

	ret = scmi_base_xfer_no_tx(SCMI_BASE_DISCOVER_SUB_VENDOR, id, sizeof(*id));
	if (ret) {
		LOG_ERR("base get subvendor id failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base subvendor id:%s", id->subvendor_id);

	return 0;
}

static int scmi_base_implementation_version_get(struct scmi_msg_base_impl_ver_p2a *impl_ver)
{
	int ret;

	ret = scmi_base_xfer_no_tx(SCMI_BASE_DISCOVER_IMPLEMENT_VERSION, impl_ver,
				   sizeof(*impl_ver));
	if (ret) {
		LOG_ERR("base get impl_ver failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base impl_ver:0x%08x", impl_ver->impl_ver);

	return ret;
}
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV */

union scmi_base_msgs_t {
	struct scmi_msg_base_attributes_p2a attr;
	struct scmi_msg_base_vendor_id_p2a vendor_id;
	struct scmi_msg_base_subvendor_id_p2a subvendor_id;
	struct scmi_msg_base_impl_ver_p2a impl_ver;
};

int scmi_base_get_revision_info(struct scmi_revision_info *rev)
{
	struct scmi_protocol_version ver;
#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)
	union scmi_base_msgs_t msgs;
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV */
	int ret;

	ret = scmi_base_get_version(&ver);
	if (ret) {
		return ret;
	}

	rev->major_ver = ver.major;
	rev->minor_ver = ver.minor;

	LOG_DBG("scmi base protocol v%04x.%04x", rev->major_ver, rev->minor_ver);

#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)
	ret = scmi_base_attributes_get(&msgs.attr);
	if (ret) {
		return ret;
	}

	rev->num_agents = msgs.attr.num_agents;
	rev->num_protocols = msgs.attr.num_protocols;

	ret = scmi_base_vendor_id_get(&msgs.vendor_id);
	if (ret) {
		return ret;
	}

	memcpy(rev->vendor_id, msgs.vendor_id.vendor_id, SCMI_SHORT_NAME_MAX_SIZE);

	ret = scmi_base_subvendor_id_get(&msgs.subvendor_id);
	if (ret) {
		return ret;
	}

	memcpy(rev->sub_vendor_id, msgs.subvendor_id.subvendor_id, SCMI_SHORT_NAME_MAX_SIZE);

	ret = scmi_base_implementation_version_get(&msgs.impl_ver);
	if (ret) {
		return ret;
	}

	LOG_DBG("scmi base revision info vendor '%s:%s' fw version 0x%x protocols:%d agents:%d",
		rev->vendor_id, rev->sub_vendor_id, rev->impl_ver, rev->num_protocols,
		rev->num_agents);
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV */

	return 0;
}

#if defined(CONFIG_ARM_SCMI_BASE_AGENT_HELPERS)
int scmi_base_discover_agent(uint32_t agent_id, struct scmi_agent_info *agent_inf)
{
	struct scmi_msg_base_discover_agent_a2p tx;
	struct scmi_msg_base_discover_agent_p2a rx;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	tx.agent_id = agent_id;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_BASE_DISCOVER_AGENT, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = sizeof(rx);
	reply.content = &rx;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("base proto discover agent failed (%d)", ret);
		return ret;
	}

	agent_inf->agent_id = rx.agent_id;
	strncpy(agent_inf->name, rx.name, SCMI_SHORT_NAME_MAX_SIZE);

	LOG_DBG("base discover agent agent_id:%u name:%s", agent_inf->agent_id, agent_inf->name);

	return 0;
}

int scmi_base_device_permission(uint32_t agent_id, uint32_t device_id, bool allow)
{
	struct scmi_msg_base_set_device_permissions_a2p tx;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	LOG_DBG("base proto agent:%u device:%u permission set allow:%d", agent_id, device_id,
		allow);

	tx.agent_id = agent_id;
	tx.device_id = device_id;
	tx.flags = allow ? SCMI_BASE_DEVICE_ACCESS_ALLOW : 0;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_BASE_SET_DEVICE_PERMISSIONS, SCMI_COMMAND, proto->id,
					0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = 0;
	reply.content = NULL;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("base agent:%u device:%u permission allow:%d failed (%d)", agent_id,
			device_id, allow, ret);
		return ret;
	}

	LOG_DBG("base  agent:%u device:%u permission set allow:%d done", agent_id, device_id,
		allow);

	return 0;
}

int scmi_base_reset_agent_cfg(uint32_t agent_id, bool reset_perm)
{
	struct scmi_msg_base_reset_agent_cfg_a2p tx;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d", agent_id, reset_perm);

	tx.agent_id = agent_id;
	tx.flags = reset_perm ? SCMI_BASE_AGENT_PERMISSIONS_RESET : 0;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SCMI_BASE_RESET_AGENT_CONFIGURATION, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(tx);
	msg.content = &tx;

	reply.len = 0;
	reply.content = NULL;

	ret = scmi_send_message(proto, &msg, &reply);
	if (ret < 0) {
		LOG_ERR("base agent:%u reset cfg failed (%d)", agent_id, ret);
		return ret;
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d done", agent_id, reset_perm);

	return 0;
}
#endif /* CONFIG_ARM_SCMI_BASE_AGENT_HELPERS */
