/*
 * System Control and Management Interface (SCMI) Base Protocol
 *
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/firmware/scmi/base.h>

LOG_MODULE_REGISTER(arm_scmi_proto_base);

DT_SCMI_PROTOCOL_DEFINE_NODEV(DT_INST(0, DT_SCMI_TRANSPORT_COMPATIBLE), NULL,
	SCMI_BASE_PROTOCOL_SUPPORTED_VERSION);

/*
 * SCMI Base protocol commands
 */
enum scmi_base_protocol_cmd {
	PROTOCOL_VERSION = 0x0,
	PROTOCOL_ATTRIBUTES = 0x1,
	PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	DISCOVER_VENDOR = 0x3,
	DISCOVER_SUB_VENDOR = 0x4,
	DISCOVER_IMPLEMENT_VERSION = 0x5,
	DISCOVER_LIST_PROTOCOLS = 0x6,
	DISCOVER_AGENT = 0x7,
	NOTIFY_ERRORS = 0x8,
	SET_DEVICE_PERMISSIONS = 0x9,
	SET_PROTOCOL_PERMISSIONS = 0xa,
	RESET_AGENT_CONFIGURATION = 0xb,
};

/* BASE PROTOCOL_ATTRIBUTES */
struct scmi_msg_base_attributes {
	uint8_t num_protocols;
	uint8_t num_agents;
	uint16_t reserved;
} __packed;

/* BASE_DISCOVER_VENDOR */
struct scmi_msg_base_vendor_id_reply {
	int32_t status;
	char vendor_id[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* BASE_DISCOVER_SUB_VENDOR */
struct scmi_msg_base_subvendor_id_reply {
	int32_t status;
	char subvendor_id[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/* BASE_DISCOVER_IMPLEMENTATION_VERSION */
struct scmi_msg_base_impl_ver_reply {
	int32_t status;
	uint32_t impl_ver;
} __packed;

/* BASE_DISCOVER_AGENT */
struct scmi_msg_base_discover_agent_reply {
	int32_t status;
	uint32_t agent_id;
	char name[SCMI_SHORT_NAME_MAX_SIZE];
} __packed;

/*
 * BASE_SET_DEVICE_PERMISSIONS
 */
#define SCMI_BASE_DEVICE_ACCESS_ALLOW			BIT(0)

struct scmi_msg_base_set_device_permissions_config {
	uint32_t agent_id;
	uint32_t device_id;
	uint32_t flags;
} __packed;

/*
 * BASE_RESET_AGENT_CONFIGURATION
 */
#define SCMI_BASE_AGENT_PERMISSIONS_RESET		BIT(0)

struct scmi_msg_base_reset_agent_cfg_config {
	uint32_t agent_id;
	uint32_t flags;
} __packed;

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

	reply.hdr = msg.hdr;
	reply.len = rx_len;
	reply.content = rx_buf;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		LOG_ERR("base xfer failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int scmi_base_vendor_id_get(struct scmi_msg_base_vendor_id_reply *id)
{
	int ret;

	ret = scmi_base_xfer_no_tx(DISCOVER_VENDOR, id, sizeof(*id));
	if (ret) {
		LOG_ERR("base get vendor id failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base vendor id:%s", id->vendor_id);

	return 0;
}

static int scmi_base_subvendor_id_get(struct scmi_msg_base_subvendor_id_reply *id)
{
	int ret;

	ret = scmi_base_xfer_no_tx(DISCOVER_SUB_VENDOR, id, sizeof(*id));
	if (ret) {
		LOG_ERR("base get subvendor id failed (%d)", ret);
		return ret;
	}

	LOG_DBG("base subvendor id:%s", id->subvendor_id);

	return 0;
}

static int scmi_base_implementation_version_get(struct scmi_msg_base_impl_ver_reply *impl_ver)
{
	int ret;

	ret = scmi_base_xfer_no_tx(DISCOVER_IMPLEMENT_VERSION, impl_ver,
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
	struct scmi_msg_base_attributes attr;
	struct scmi_msg_base_vendor_id_reply vendor_id;
	struct scmi_msg_base_subvendor_id_reply subvendor_id;
	struct scmi_msg_base_impl_ver_reply impl_ver;
};

int scmi_base_get_revision_info(struct scmi_revision_info *rev)
{
	struct scmi_protocol_version version;
#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)
	union scmi_base_msgs_t msgs;
	uint32_t attr_val;
#endif /* CONFIG_ARM_SCMI_BASE_EXT_REV */
	int ret;

	struct scmi_protocol *proto;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	ret = scmi_protocol_get_version(proto, &(version.raw));
	if (ret) {
		return ret;
	}

	rev->major_ver = version.major;
	rev->minor_ver = version.minor;

	LOG_DBG("scmi base protocol v%04x.%04x", rev->major_ver, rev->minor_ver);

#if defined(CONFIG_ARM_SCMI_BASE_EXT_REV)
	ret = scmi_protocol_attributes_get(proto, &attr_val);
	if (ret) {
		return ret;
	}

	memcpy(&msgs.attr, &attr_val, sizeof(msgs.attr));
	rev->num_agents = msgs.attr.num_agents;
	rev->num_protocols = msgs.attr.num_protocols;

	ret = scmi_base_vendor_id_get(&msgs.vendor_id);
	if (ret) {
		return ret;
	}

	if (msgs.vendor_id.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(msgs.vendor_id.status);
	}

	memcpy(rev->vendor_id, msgs.vendor_id.vendor_id, SCMI_SHORT_NAME_MAX_SIZE);

	ret = scmi_base_subvendor_id_get(&msgs.subvendor_id);
	if (ret) {
		return ret;
	}

	if (msgs.subvendor_id.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(msgs.subvendor_id.status);
	}

	memcpy(rev->sub_vendor_id, msgs.subvendor_id.subvendor_id, SCMI_SHORT_NAME_MAX_SIZE);

	ret = scmi_base_implementation_version_get(&msgs.impl_ver);
	if (ret) {
		return ret;
	}

	if (msgs.impl_ver.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(msgs.impl_ver.status);
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
	struct scmi_msg_base_discover_agent_reply reply_buffer;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(DISCOVER_AGENT, SCMI_COMMAND, proto->id, 0x0);
	msg.len = sizeof(agent_id);
	msg.content = &agent_id;

	reply.hdr = msg.hdr;
	reply.len = sizeof(struct scmi_msg_base_discover_agent_reply);
	reply.content = &reply_buffer;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		LOG_ERR("base proto discover agent failed (%d)", ret);
		return ret;
	}

	if (reply_buffer.status != SCMI_SUCCESS) {
		return scmi_status_to_errno(reply_buffer.status);
	}

	agent_inf->agent_id = reply_buffer.agent_id;
	strncpy(agent_inf->name, reply_buffer.name, SCMI_SHORT_NAME_MAX_SIZE);

	LOG_DBG("base discover agent agent_id:%u name:%s", agent_inf->agent_id, agent_inf->name);

	return 0;
}

int scmi_base_device_permission(uint32_t agent_id, uint32_t device_id, bool allow)
{
	struct scmi_msg_base_set_device_permissions_config cfg;
	int32_t status;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	LOG_DBG("base proto agent:%u device:%u permission set allow:%d", agent_id, device_id,
		allow);

	cfg.agent_id = agent_id;
	cfg.device_id = device_id;
	cfg.flags = allow ? SCMI_BASE_DEVICE_ACCESS_ALLOW : 0;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(SET_DEVICE_PERMISSIONS, SCMI_COMMAND, proto->id,
					0x0);
	msg.len = sizeof(cfg);
	msg.content = &cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		LOG_ERR("base agent:%u device:%u permission allow:%d failed (%d)", agent_id,
			device_id, allow, ret);
		return ret;
	}

	if (status != SCMI_SUCCESS) {
		return scmi_status_to_errno(status);
	}

	LOG_DBG("base  agent:%u device:%u permission set allow:%d done", agent_id, device_id,
		allow);

	return 0;
}

int scmi_base_reset_agent_cfg(uint32_t agent_id, bool reset_perm)
{
	struct scmi_msg_base_reset_agent_cfg_config cfg;
	int32_t status;
	struct scmi_message msg, reply;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d", agent_id, reset_perm);

	cfg.agent_id = agent_id;
	cfg.flags = reset_perm ? SCMI_BASE_AGENT_PERMISSIONS_RESET : 0;

	msg.hdr = SCMI_MESSAGE_HDR_MAKE(RESET_AGENT_CONFIGURATION, SCMI_COMMAND,
					proto->id, 0x0);
	msg.len = sizeof(cfg);
	msg.content = &cfg;

	reply.hdr = msg.hdr;
	reply.len = sizeof(status);
	reply.content = &status;

	ret = scmi_send_message(proto, &msg, &reply, false);
	if (ret < 0) {
		LOG_ERR("base agent:%u reset cfg failed (%d)", agent_id, ret);
		return ret;
	}

	if (status != SCMI_SUCCESS) {
		return scmi_status_to_errno(status);
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d done", agent_id, reset_perm);

	return 0;
}
#endif /* CONFIG_ARM_SCMI_BASE_AGENT_HELPERS */
