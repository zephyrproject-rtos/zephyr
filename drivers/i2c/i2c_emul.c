/*
 * This driver creates fake I2C buses which can contain emulated devices,
 * implemented by a separate emulation driver. The API between this driver and
 * its emulators is defined by struct i2c_emul_driver_api.
 *
 * Copyright 2020 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_i2c_emul_controller

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_emul_ctlr);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#include "i2c-priv.h"

/** Working data for the device */
struct i2c_emul_data {
	/* List of struct i2c_emul associated with the device */
	sys_slist_t emuls;
	/* I2C host configuration */
	uint32_t config;
	uint32_t bitrate;
};

/**
 * Find an emulator by its I2C address
 *
 * @param dev I2C emulation controller device
 * @param addr I2C address of that device
 * @return emulator ro use
 * @return NULL if not found
 */
static struct i2c_emul *i2c_emul_find(const struct device *dev, int addr)
{
	struct i2c_emul_data *data = dev->data;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->emuls, node) {
		struct i2c_emul *emul = NULL;

		emul = CONTAINER_OF(node, struct i2c_emul, node);
		if (emul->addr == addr) {
			return emul;
		}
	}

	return NULL;
}

static int i2c_emul_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_emul_data *data = dev->data;

	data->config = dev_config;

	return 0;
}

static int i2c_emul_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_emul_data *data = dev->data;

	*dev_config = data->config;

	return 0;
}

static int i2c_emul_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t addr)
{
	struct i2c_emul *emul;
	const struct i2c_emul_api *api;
	int ret;

	emul = i2c_emul_find(dev, addr);
	if (!emul) {
		return -EIO;
	}

	api = emul->api;
	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->transfer);

	if (emul->mock_api != NULL && emul->mock_api->transfer != NULL) {
		ret = emul->mock_api->transfer(emul->target, msgs, num_msgs, addr);
		if (ret != -ENOSYS) {
			return ret;
		}
	}

	return api->transfer(emul->target, msgs, num_msgs, addr);
}

/**
 * Set up a new emulator and add it to the list
 *
 * @param dev I2C emulation controller device
 */
static int i2c_emul_init(const struct device *dev)
{
	struct i2c_emul_data *data = dev->data;
	int rc;

	sys_slist_init(&data->emuls);

	rc = emul_init_for_bus(dev);

	/* Set config to an uninitialized state */
	data->config = (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(data->bitrate));

	return rc;
}

int i2c_emul_register(const struct device *dev, struct i2c_emul *emul)
{
	struct i2c_emul_data *data = dev->data;
	const char *name = emul->target->dev->name;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s' at I2C addr %02x", name, emul->addr);

	return 0;
}

/* Device instantiation */

static const struct i2c_driver_api i2c_emul_api = {
	.configure = i2c_emul_configure,
	.get_config = i2c_emul_get_config,
	.transfer = i2c_emul_transfer,
};

#define EMUL_LINK_AND_COMMA(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	},

#define I2C_EMUL_INIT(n)                                                                           \
	static const struct emul_link_for_bus emuls_##n[] = {                                      \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};                \
	static struct emul_list_for_bus i2c_emul_cfg_##n = {                                       \
		.children = emuls_##n,                                                             \
		.num_children = ARRAY_SIZE(emuls_##n),                                             \
	};                                                                                         \
	static struct i2c_emul_data i2c_emul_data_##n = {                                          \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_emul_init, NULL, &i2c_emul_data_##n, &i2c_emul_cfg_##n,   \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_emul_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_EMUL_INIT)
