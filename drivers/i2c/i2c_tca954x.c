/*
 * Copyright (c) 2020 Innoseis BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

LOG_MODULE_REGISTER(tca954x, CONFIG_I2C_LOG_LEVEL);

struct tca954x_root_config {
	struct i2c_dt_spec i2c;
	uint8_t nchans;
	const struct gpio_dt_spec reset_gpios;
};

struct tca954x_root_data {
	struct k_mutex lock;
	uint8_t selected_chan;
};

struct tca954x_channel_config {
	const struct device *root;
	uint8_t chan_mask;
};

static inline struct tca954x_root_data *
get_root_data_from_channel(const struct device *dev)
{
	const struct tca954x_channel_config *channel_config = dev->config;

	return channel_config->root->data;
}

static inline const struct tca954x_root_config *
get_root_config_from_channel(const struct device *dev)
{
	const struct tca954x_channel_config *channel_config = dev->config;

	return channel_config->root->config;
}

static int tca954x_configure(const struct device *dev, uint32_t dev_config)
{
	const struct tca954x_root_config *cfg =
			get_root_config_from_channel(dev);

	return i2c_configure(cfg->i2c.bus, dev_config);
}

static int tca954x_set_channel(const struct device *dev, uint8_t select_mask)
{
	int res = 0;
	struct tca954x_root_data *data = dev->data;
	const struct tca954x_root_config *cfg = dev->config;

	/* Only select the channel if its different from the last channel */
	if (data->selected_chan != select_mask) {
		res = i2c_write_dt(&cfg->i2c, &select_mask, 1);
		if (res == 0) {
			data->selected_chan = select_mask;
		} else {
			LOG_DBG("tca954x: failed to set channel");
		}
	}
	return res;
}

static int tca954x_transfer(const struct device *dev,
			     struct i2c_msg *msgs,
			     uint8_t num_msgs,
			     uint16_t addr)
{
	struct tca954x_root_data *data = get_root_data_from_channel(dev);
	const struct tca954x_root_config *config =
			get_root_config_from_channel(dev);
	const struct tca954x_channel_config *down_cfg = dev->config;
	int res;

	res = k_mutex_lock(&data->lock, K_MSEC(5000));
	if (res != 0) {
		return res;
	}

	res = tca954x_set_channel(down_cfg->root, down_cfg->chan_mask);
	if (res != 0) {
		goto end_trans;
	}

	res = i2c_transfer(config->i2c.bus, msgs, num_msgs, addr);

end_trans:
	k_mutex_unlock(&data->lock);
	return res;
}

static int tca954x_root_init(const struct device *dev)
{
	struct tca954x_root_data *i2c_tca954x = dev->data;
	const struct tca954x_root_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	/* If the RESET line is available, configure it. */
	if (config->reset_gpios.port) {
		if (!device_is_ready(config->reset_gpios.port)) {
			LOG_ERR("%s is not ready",
				config->reset_gpios.port->name);
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->reset_gpios, GPIO_OUTPUT)) {
			LOG_ERR("%s: failed to configure RESET line", dev->name);
			return -EIO;
		}

		/* Deassert reset line */
		gpio_pin_set_dt(&config->reset_gpios, 0);
	}

	i2c_tca954x->selected_chan = 0;

	return 0;
}

static int tca954x_channel_init(const struct device *dev)
{
	const struct tca954x_channel_config *chan_cfg = dev->config;
	const struct tca954x_root_config *root_cfg =
			get_root_config_from_channel(dev);

	if (!device_is_ready(chan_cfg->root)) {
		LOG_ERR("I2C mux root %s not ready", chan_cfg->root->name);
		return -ENODEV;
	}

	if (chan_cfg->chan_mask >= BIT(root_cfg->nchans)) {
		LOG_ERR("Wrong DTS address provided for %s", dev->name);
		return -EINVAL;
	}

	return 0;
}

const struct i2c_driver_api tca954x_api_funcs = {
	.configure = tca954x_configure,
	.transfer = tca954x_transfer,
};

#define TCA954x_CHILD_DEFINE(node_id, n)				    \
	static const struct tca954x_channel_config			    \
		tca##n##a_down_config_##node_id = {			    \
		.chan_mask = BIT(DT_REG_ADDR(node_id)),			    \
		.root = DEVICE_DT_GET(DT_PARENT(node_id)),		    \
	};								    \
	DEVICE_DT_DEFINE(node_id,					    \
			 tca954x_channel_init,				    \
			 NULL,						    \
			 NULL,						    \
			 &tca##n##a_down_config_##node_id,		    \
			 POST_KERNEL, CONFIG_I2C_TCA954X_CHANNEL_INIT_PRIO, \
			 &tca954x_api_funcs);

#define TCA954x_ROOT_DEFINE(n, inst, ch)				          \
	static const struct tca954x_root_config tca##n##a_cfg_##inst = {          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),				  \
		.nchans = ch,							  \
		.reset_gpios = GPIO_DT_SPEC_GET_OR(			          \
				DT_INST(inst, ti_tca##n##a), reset_gpios, {0}),	  \
	};								          \
	static struct tca954x_root_data tca##n##a_data_##inst = {		  \
		.lock = Z_MUTEX_INITIALIZER(tca##n##a_data_##inst.lock),	  \
	};									  \
	I2C_DEVICE_DT_DEFINE(DT_INST(inst, ti_tca##n##a),			  \
			      tca954x_root_init, NULL,				  \
			      &tca##n##a_data_##inst, &tca##n##a_cfg_##inst,	  \
			      POST_KERNEL, CONFIG_I2C_TCA954X_ROOT_INIT_PRIO,	  \
			      NULL);						  \
	DT_FOREACH_CHILD_VARGS(DT_INST(inst, ti_tca##n##a), TCA954x_CHILD_DEFINE, n);

/*
 * TCA9546A: 4 channels
 */
#define TCA9546A_INIT(n) TCA954x_ROOT_DEFINE(9546, n, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tca9546a
DT_INST_FOREACH_STATUS_OKAY(TCA9546A_INIT)

/*
 * TCA9548A: 8 channels
 */
#define TCA9548A_INIT(n) TCA954x_ROOT_DEFINE(9548, n, 8)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tca9548a
DT_INST_FOREACH_STATUS_OKAY(TCA9548A_INIT)
