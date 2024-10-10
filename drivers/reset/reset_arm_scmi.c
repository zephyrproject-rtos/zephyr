/*
 * Copyright 2024 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/firmware/scmi/reset.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(reset_arm_scmi);

#define DT_DRV_COMPAT arm_scmi_reset

struct scmi_reset_drv_data {
	struct scmi_protocol_version version;
	uint16_t num_domains;
};

static int scmi_reset_line_assert(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain_assert(proto, id);
}

static int scmi_reset_line_deassert(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain_deassert(proto, id);
}

static int scmi_reset_line_toggle(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain_toggle(proto, id);
}

static int scmi_reset_init(const struct device *dev)
{
	struct scmi_protocol *proto;
	struct scmi_reset_drv_data *data;
	int ret;

	proto = dev->data;
	data = proto->data;

	ret = scmi_core_get_version(proto, &data->version);
	if (ret) {
		return ret;
	}

	if (data->version.major != SCMI_PROTOCOL_RESET_REV_MAJOR) {
		LOG_ERR("unsupported reset protocol version 0x%04x.0x%04x ", data->version.major,
			data->version.minor);
		return -ENOTSUP;
	}

	ret = scmi_reset_get_attr(proto, &data->num_domains);
	if (ret) {
		return ret;
	}

	LOG_INF("scmi reset rotocol version 0x%04x.0x%04x num_domains:%u", data->version.major,
		data->version.minor, data->num_domains);

	return 0;
}

static const struct reset_driver_api scmi_reset_driver_api = {
	.line_assert = scmi_reset_line_assert,
	.line_deassert = scmi_reset_line_deassert,
	.line_toggle = scmi_reset_line_toggle,
};

static struct scmi_reset_drv_data scmi_reset_data;

DT_INST_SCMI_PROTOCOL_DEFINE(0, &scmi_reset_init, NULL, &scmi_reset_data, NULL,
			     PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
			     &scmi_reset_driver_api);
