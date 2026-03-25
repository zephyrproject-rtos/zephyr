/*
 * Copyright 2026 EPAM Systems
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
	uint16_t num_domains;
};

static int reset_scmi_line_assert(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain(proto, id, SCMI_RESET_EXPLICIT_ASSERT, SCMI_RESET_ARCH_COLD_RESET);
}

static int reset_scmi_line_deassert(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain(proto, id, 0, SCMI_RESET_ARCH_COLD_RESET);
}

static int reset_scmi_line_toggle(const struct device *dev, uint32_t id)
{
	struct scmi_reset_drv_data *data;
	struct scmi_protocol *proto;

	proto = dev->data;
	data = proto->data;

	if (id >= data->num_domains) {
		return -EINVAL;
	}

	return scmi_reset_domain(proto, id, SCMI_RESET_AUTONOMOUS, SCMI_RESET_ARCH_COLD_RESET);
}

static int reset_scmi_init(const struct device *dev)
{
	struct scmi_protocol *proto;
	struct scmi_reset_drv_data *data;
	uint32_t attributes;
	int ret;

	proto = dev->data;
	data = proto->data;

	ret = scmi_protocol_attributes_get(proto, &attributes);
	if (ret != 0) {
		return ret;
	}

	data->num_domains = SCMI_RESET_ATTR_GET_NUM_DOMAINS(attributes);

	LOG_INF("scmi reset protocol num_domains:%u", data->num_domains);

	return 0;
}

static DEVICE_API(reset, reset_arm_scmi_driver_api) = {
	.line_assert = reset_scmi_line_assert,
	.line_deassert = reset_scmi_line_deassert,
	.line_toggle = reset_scmi_line_toggle,
};

static struct scmi_reset_drv_data scmi_reset_data;

DT_INST_SCMI_PROTOCOL_DEFINE(0, &reset_scmi_init, NULL, &scmi_reset_data, NULL,
			     PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY,
			     &reset_arm_scmi_driver_api, SCMI_RESET_PROTOCOL_SUPPORTED_VERSION);
