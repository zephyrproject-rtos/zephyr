/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#define DT_DRV_COMPAT zephyr_mctp_i3c_endpoint

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/pmci/mctp/mctp_i3c_endpoint.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mctp_i3c_endpoint, CONFIG_MCTP_LOG_LEVEL);

struct endpoint_cfg {
	const struct device *bus;
	struct i3c_device_id i3c_id;
};

struct endpoint_data {
	struct i3c_device_desc *i3c_dev;
	struct mctp_binding_i3c_controller *binding;
};


static void endpoint_bind(const struct device *dev, struct mctp_binding_i3c_controller *binding,
			  struct i3c_device_desc **i3c_dev)
{
	struct endpoint_data *data = dev->data;

	data->binding = binding;
	*i3c_dev = data->i3c_dev;
}


static struct mctp_binding_i3c_controller *endpoint_binding(const struct device *dev)
{
	struct endpoint_data *data = dev->data;

	return data->binding;
}

static int endpoint_init(const struct device *dev)
{
	struct endpoint_data *data = dev->data;
	const struct endpoint_cfg *cfg = dev->config;

	data->i3c_dev = i3c_device_find(cfg->bus, &cfg->i3c_id);

	if (data->i3c_dev == NULL) {
		LOG_ERR("Cannot find I3C device descriptor");
		return -ENODEV;
	}

	return 0;
}

static const struct mctp_i3c_endpoint_api endpoint_api = {
	.bind = endpoint_bind,
	.binding = endpoint_binding,
};

/* clang-format off */
#define ENDPOINT_DEVICE(_inst) \
	static const struct endpoint_cfg _endpoint_cfg_##_inst = { \
		.bus = DEVICE_DT_GET(DT_INST_BUS(_inst)), \
		.i3c_id = I3C_DEVICE_ID_DT_INST(_inst), \
	}; \
	static struct endpoint_data _endpoint_data_##_inst; \
	DEVICE_DT_INST_DEFINE(_inst, \
			      endpoint_init, \
			      NULL, \
			      &_endpoint_data_##_inst, \
			      &_endpoint_cfg_##_inst, \
			      POST_KERNEL, \
			      CONFIG_I3C_CONTROLLER_INIT_PRIORITY, \
			      &endpoint_api);

DT_INST_FOREACH_STATUS_OKAY(ENDPOINT_DEVICE)

/* clang-format on */
