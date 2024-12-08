/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(arm_scmi_clock);

#define DT_DRV_COMPAT arm_scmi_clock

struct scmi_clock_data {
	uint32_t clk_num;
};

static int scmi_clock_on_off(const struct device *dev,
			     clock_control_subsys_t clk, bool on)
{
	struct scmi_clock_data *data;
	struct scmi_protocol *proto;
	uint32_t clk_id;
	struct scmi_clock_config cfg;

	proto = dev->data;
	data = proto->data;
	clk_id = POINTER_TO_UINT(clk);

	if (clk_id >= data->clk_num) {
		return -EINVAL;
	}

	memset(&cfg, 0, sizeof(cfg));

	cfg.attributes = SCMI_CLK_CONFIG_ENABLE_DISABLE(on);
	cfg.clk_id = clk_id;

	return scmi_clock_config_set(proto, &cfg);
}

static int scmi_clock_on(const struct device *dev, clock_control_subsys_t clk)
{
	return scmi_clock_on_off(dev, clk, true);
}

static int scmi_clock_off(const struct device *dev, clock_control_subsys_t clk)
{
	return scmi_clock_on_off(dev, clk, false);
}

static int scmi_clock_get_rate(const struct device *dev,
			       clock_control_subsys_t clk, uint32_t *rate)
{
	struct scmi_clock_data *data;
	struct scmi_protocol *proto;
	uint32_t clk_id;

	proto = dev->data;
	data = proto->data;
	clk_id = POINTER_TO_UINT(clk);

	if (clk_id >= data->clk_num) {
		return -EINVAL;
	}

	return scmi_clock_rate_get(proto, clk_id, rate);
}

static DEVICE_API(clock_control, scmi_clock_api) = {
	.on = scmi_clock_on,
	.off = scmi_clock_off,
	.get_rate = scmi_clock_get_rate,
};

static int scmi_clock_init(const struct device *dev)
{
	struct scmi_protocol *proto;
	struct scmi_clock_data *data;
	int ret;
	uint32_t attributes;

	proto = dev->data;
	data = proto->data;

	ret = scmi_clock_protocol_attributes(proto, &attributes);
	if (ret < 0) {
		LOG_ERR("failed to fetch clock attributes: %d", ret);
		return ret;
	}

	data->clk_num = SCMI_CLK_ATTRIBUTES_CLK_NUM(attributes);

	return 0;
}

static struct scmi_clock_data data;

DT_INST_SCMI_PROTOCOL_DEFINE(0, &scmi_clock_init, NULL, &data, NULL,
			     PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
			     &scmi_clock_api);
