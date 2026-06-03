/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/clk.h>
#include <string.h>
#include <zephyr/kernel.h>

/* TODO: if extended attributes are supported this should be moved
 * to the header file so that users will have access to it.
 */
#define SCMI_CLK_CONFIG_EA_MASK GENMASK(23, 16)

enum scmi_clock_message {
	CLOCK_ATTRIBUTES = 0x3,
	CLOCK_DESCRIBE_RATES = 0x4,
	CLOCK_RATE_SET = 0x5,
	CLOCK_RATE_GET = 0x6,
	CLOCK_CONFIG_SET = 0x7,
	CLOCK_NAME_GET = 0x8,
	CLOCK_RATE_NOTIFY = 0x9,
	CLOCK_RATE_CHANGE_REQUESTED_NOTIFY = 0xa,
	CLOCK_CONFIG_GET = 0xb,
	CLOCK_POSSIBLE_PARENTS_GET = 0xc,
	CLOCK_PARENT_SET = 0xd,
	CLOCK_PARENT_GET = 0xe,
	CLOCK_GET_PERMISSIONS = 0xf,
};

struct scmi_clock_parent_config {
	uint32_t clk_id;
	uint32_t parent_id;
};

int scmi_clock_rate_get(struct scmi_protocol *proto,
			uint32_t clk_id, uint32_t *rate)
{
	struct scmi_xfer xfer;
	uint64_t clk_rate;
	int ret;

	/* input validation */
	if (!proto || !rate) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_RATE_GET,
			     &clk_id, sizeof(clk_id),
			     &clk_rate, sizeof(clk_rate));
	if (ret < 0) {
		return ret;
	}

	ret = scmi_send_message(proto, &xfer);
	if (ret < 0) {
		return ret;
	}

	*rate = (uint32_t)clk_rate;

	return 0;
}

int scmi_clock_rate_set(struct scmi_protocol *proto, struct scmi_clock_rate_config *cfg)
{
	struct scmi_xfer xfer;
	int ret;

	/* input validation */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	/* Currently ASYNC flag is not supported. */
	if (cfg->flags & SCMI_CLK_RATE_SET_FLAGS_ASYNC) {
		return -ENOTSUP;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_RATE_SET,
			     cfg, sizeof(*cfg), NULL, 0x0);
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_clock_parent_get(struct scmi_protocol *proto, uint32_t clk_id, uint32_t *parent_id)
{
	struct scmi_xfer xfer;
	int ret;

	/* input validation */
	if (!proto || !parent_id) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_PARENT_GET,
			     &clk_id, sizeof(clk_id),
			     parent_id, sizeof(*parent_id));
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_clock_parent_set(struct scmi_protocol *proto, uint32_t clk_id, uint32_t parent_id)
{
	struct scmi_clock_parent_config cfg = {.clk_id = clk_id, .parent_id = parent_id};
	struct scmi_xfer xfer;
	int ret;

	/* input validation */
	if (!proto) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_PARENT_SET,
			     &cfg, sizeof(cfg), NULL, 0x0);
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_clock_config_set(struct scmi_protocol *proto,
			  struct scmi_clock_config *cfg)
{
	struct scmi_xfer xfer;
	int ret;

	/* input validation */
	if (!proto || !cfg) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	/* extended attributes currently not supported */
	if (cfg->attributes & SCMI_CLK_CONFIG_EA_MASK) {
		return -ENOTSUP;
	}

	/* invalid because extended attributes are not supported */
	if (SCMI_CLK_CONFIG_ENABLE_DISABLE(cfg->attributes) == 3) {
		return -ENOTSUP;
	}

	/* this is a reserved value */
	if (SCMI_CLK_CONFIG_ENABLE_DISABLE(cfg->attributes) == 2) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_CONFIG_SET,
			     cfg, sizeof(*cfg), NULL, 0x0);
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_clock_attributes(struct scmi_protocol *proto, uint32_t clk_id,
			  struct scmi_clock_attributes *attributes)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto || !attributes) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_ATTRIBUTES,
			     &clk_id, sizeof(clk_id),
			     attributes, sizeof(*attributes));
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}

int scmi_clock_get_permissions(struct scmi_protocol *proto, uint32_t clk_id,
			       uint32_t *permissions)
{
	struct scmi_xfer xfer;
	int ret;

	if (!proto || !permissions) {
		return -EINVAL;
	}

	if (proto->id != SCMI_PROTOCOL_CLOCK) {
		return -EINVAL;
	}

	ret = scmi_xfer_init(proto, &xfer, CLOCK_GET_PERMISSIONS,
			     &clk_id, sizeof(clk_id),
			     permissions, sizeof(*permissions));
	if (ret < 0) {
		return ret;
	}

	return scmi_send_message(proto, &xfer);
}
