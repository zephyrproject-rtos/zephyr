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

static int scmi_base_xfer_no_tx(uint8_t msg_id, void *rx_buf, size_t rx_len)
{
	struct scmi_xfer xfer;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, msg_id,
			     NULL, 0x0, rx_buf, rx_len);
	if (ret < 0) {
		return ret;
	}

	ret = scmi_send_message(proto, &xfer);
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

union scmi_base_msgs_t {
	struct scmi_msg_base_attributes attr;
	struct scmi_msg_base_vendor_id_reply vendor_id;
	struct scmi_msg_base_subvendor_id_reply subvendor_id;
	struct scmi_msg_base_impl_ver_reply impl_ver;
};

int scmi_base_get_revision_info(struct scmi_revision_info *rev)
{
	struct scmi_protocol_version version;
	union scmi_base_msgs_t msgs;
	uint32_t attr_val;
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

	return 0;
}

int scmi_base_discover_agent(uint32_t agent_id, struct scmi_agent_info *agent_inf)
{
	struct scmi_msg_base_discover_agent_reply reply_buffer;
	struct scmi_xfer xfer;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, DISCOVER_AGENT,
			     &agent_id, sizeof(agent_id),
			     &reply_buffer, sizeof(reply_buffer));
	if (ret < 0) {
		return ret;
	}

	ret = scmi_send_message(proto, &xfer);
	if (ret < 0) {
		LOG_ERR("base proto discover agent failed (%d)", ret);
		return ret;
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
	struct scmi_xfer xfer;
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

	ret = scmi_xfer_init(proto, &xfer, SET_DEVICE_PERMISSIONS,
			     &cfg, sizeof(cfg), &status, sizeof(status));
	if (ret < 0) {
		return ret;
	}

	ret = scmi_send_message(proto, &xfer);
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
	struct scmi_msg_base_reset_agent_cfg_config cfg;
	int32_t status;
	struct scmi_xfer xfer;
	struct scmi_protocol *proto;
	int ret;

	proto = &SCMI_PROTOCOL_NAME(SCMI_PROTOCOL_BASE);
	if (!proto) {
		return -EINVAL;
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d", agent_id, reset_perm);

	cfg.agent_id = agent_id;
	cfg.flags = reset_perm ? SCMI_BASE_AGENT_PERMISSIONS_RESET : 0;

	ret = scmi_xfer_init(proto, &xfer, RESET_AGENT_CONFIGURATION,
			     &cfg, sizeof(cfg), &status, sizeof(status));
	if (ret < 0) {
		return ret;
	}

	ret = scmi_send_message(proto, &xfer);
	if (ret < 0) {
		LOG_ERR("base agent:%u reset cfg failed (%d)", agent_id, ret);
		return ret;
	}

	LOG_DBG("base agent:%u reset cfg reset_perm:%d done", agent_id, reset_perm);

	return 0;
}
