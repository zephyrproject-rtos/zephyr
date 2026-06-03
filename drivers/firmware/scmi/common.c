/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI Common Protocol Commands Implementation
 *
 * This file contains the implementation of the SCMI commands that are
 * common to all protocols (generic or vendor-specific) as listed in
 * ARM SCMI Specification such as v4.0 (DEN0056F), Section 3.2.2 "Base protocol Commands".
 *
 * The following common commands are implemented:
 * - PROTOCOL_VERSION (0x0): Query protocol version
 * - PROTOCOL_ATTRIBUTES (0x1): Get protocol-specific attributes
 * - MESSAGE_ATTRIBUTES (0x2): Query message capabilities
 * - NEGOTIATE_PROTOCOL_VERSION (0x10): Negotiate protocol version support
 *
 * These commands provide standardized interfaces that can be reused across
 * different SCMI protocol implementations, ensuring consistency and reducing
 * code duplication.
 *
 * Reference: ARM System Control and Management Interface Platform Design Document
 * Version 4.0, Document number: DEN0056F
 * Available at: https://developer.arm.com/documentation/den0056/latest
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/firmware/scmi/protocol.h>

enum scmi_common_cmd {
	PROTOCOL_VERSION = 0x0,
	PROTOCOL_ATTRIBUTES = 0x1,
	PROTOCOL_MESSAGE_ATTRIBUTES = 0x2,
	NEGOTIATE_PROTOCOL_VERSION = 0x10,
};

int scmi_protocol_get_version(struct scmi_protocol *proto, uint32_t *version)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto || !version) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, PROTOCOL_VERSION,
			     NULL, 0x0, version, sizeof(*version));
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_protocol_attributes_get(struct scmi_protocol *proto, uint32_t *attributes)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto || !attributes) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, PROTOCOL_ATTRIBUTES,
			     NULL, 0x0, attributes, sizeof(*attributes));
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_protocol_message_attributes_get(struct scmi_protocol *proto,
					uint32_t message_id, uint32_t *attributes)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto || !attributes) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, PROTOCOL_MESSAGE_ATTRIBUTES,
			     &message_id, sizeof(message_id),
			     attributes, sizeof(*attributes));
	if (ret < 0) {
		return ret;
	}

	xfer.use_polling = true;

	return scmi_send_message(proto, &xfer);
}

int scmi_protocol_version_negotiate(struct scmi_protocol *proto, uint32_t version)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, NEGOTIATE_PROTOCOL_VERSION,
			     &version, sizeof(version), NULL, 0x0);
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}
