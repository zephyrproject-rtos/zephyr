/*
 * Copyright (c) 2020 Innoseis BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tca9546a

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <stdint.h>

LOG_MODULE_REGISTER(tca9546a, CONFIG_I2C_LOG_LEVEL);

#define MAX_CHANNEL_MASK BIT_MASK(4)

struct tca9546a_root_config {
	const struct device *bus;
	uint16_t slave_addr;
};

struct tca9546a_root_data {
	struct k_mutex lock;
	uint8_t selected_chan;
};

struct tca9546a_channel_config {
	const struct device *root;
	uint8_t chan_mask;
};

static inline struct tca9546a_root_data *
get_root_data_from_channel(const struct device *dev)
{
	const struct tca9546a_channel_config *channel_config = dev->config;

	return channel_config->root->data;
}

static inline const struct tca9546a_root_config *
get_root_config_from_channel(const struct device *dev)
{
	const struct tca9546a_channel_config *channel_config = dev->config;

	return channel_config->root->config;
}

static int tca9546a_configure(const struct device *dev, uint32_t dev_config)
{
	const struct tca9546a_root_config *cfg = get_root_config_from_channel(
		dev);

	return i2c_configure(cfg->bus, dev_config);
}

static int tca9546a_set_channel(const struct device *dev, uint8_t select_mask)
{
	int res = 0;
	struct tca9546a_root_data *data = dev->data;
	const struct tca9546a_root_config *cfg = dev->config;

	if (data->selected_chan != select_mask) {
		res = i2c_write(cfg->bus, &select_mask, 1, cfg->slave_addr);
		if (res == 0) {
			data->selected_chan = select_mask;
		}
	}
	return res;
}

static int tca9546a_transfer(const struct device *dev,
			     struct i2c_msg *msgs,
			     uint8_t num_msgs,
			     uint16_t addr)
{
	struct tca9546a_root_data *data = get_root_data_from_channel(dev);
	const struct tca9546a_root_config *config = get_root_config_from_channel(
		dev);
	const struct tca9546a_channel_config *down_cfg = dev->config;
	int res;

	res = k_mutex_lock(&data->lock, K_MSEC(5000));
	if (res != 0) {
		return res;
	}

	res = tca9546a_set_channel(down_cfg->root, down_cfg->chan_mask);
	if (res != 0) {
		goto end_trans;
	}

	res = i2c_transfer(config->bus, msgs, num_msgs, addr);

end_trans:
	k_mutex_unlock(&data->lock);
	return res;
}

static int tca9546_root_init(const struct device *dev)
{
	struct tca9546a_root_data *i2c_tca9546a = dev->data;
	const struct tca9546a_root_config *config = dev->config;

	if (!device_is_ready(config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus->name);
		return -ENODEV;
	}

	i2c_tca9546a->selected_chan = 0;

	return 0;
}

static int tca9546a_channel_init(const struct device *dev)
{
	const struct tca9546a_channel_config *cfg = dev->config;

	if (!device_is_ready(cfg->root)) {
		LOG_ERR("I2C mux root %s not ready", cfg->root->name);
		return -ENODEV;
	}

	if (cfg->chan_mask > MAX_CHANNEL_MASK) {
		LOG_ERR("Wrong DTS address provided for %s", dev->name);
		return -EINVAL;
	}

	return 0;
}

const struct i2c_driver_api tca9546a_api_funcs = {
	.configure = tca9546a_configure,
	.transfer = tca9546a_transfer,
};

#define TCA9546A_CHILD_DEFINE(node_id)					    \
	static const struct tca9546a_channel_config			    \
		tca9546a_down_config_##node_id = {			    \
		.chan_mask = BIT(DT_REG_ADDR(node_id)),			    \
		.root = DEVICE_DT_GET(DT_PARENT(node_id)),		    \
	};								    \
	DEVICE_DT_DEFINE(node_id,					    \
			 tca9546a_channel_init,				    \
			 NULL,						    \
			 NULL,						    \
			 &tca9546a_down_config_##node_id,		    \
			 POST_KERNEL, CONFIG_I2C_TCA9546_CHANNEL_INIT_PRIO, \
			 &tca9546a_api_funcs);

#define TCA9546A_ROOT_CHILD_DEFINE(inst)				      \
	static const struct tca9546a_root_config tca9546a_cfg_##inst = {      \
		.slave_addr = DT_INST_REG_ADDR(inst),			      \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst))			      \
	};								      \
	static struct tca9546a_root_data tca9546a_data_##inst = {	      \
		.lock = Z_MUTEX_INITIALIZER(tca9546a_data_##inst.lock),	      \
	};								      \
	I2C_DEVICE_DT_INST_DEFINE(inst,					      \
			      tca9546_root_init, NULL,			      \
			      &tca9546a_data_##inst, &tca9546a_cfg_##inst,    \
			      POST_KERNEL, CONFIG_I2C_TCA9546_ROOT_INIT_PRIO, \
			      NULL);					      \
	DT_INST_FOREACH_CHILD(inst, TCA9546A_CHILD_DEFINE);

DT_INST_FOREACH_STATUS_OKAY(TCA9546A_ROOT_CHILD_DEFINE)
