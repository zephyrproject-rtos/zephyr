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
	/* Control register value after power-on or reset */
	uint8_t por_chan;
	const struct gpio_dt_spec reset_gpios;
	bool idle_disconnect;
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
	int res, disconnect_res;

	res = k_mutex_lock(&data->lock, K_MSEC(5000));
	if (res != 0) {
		return res;
	}

	res = tca954x_set_channel(down_cfg->root, down_cfg->chan_mask);
	if (res != 0) {
		goto end_trans;
	}

	res = i2c_transfer(config->i2c.bus, msgs, num_msgs, addr);

	if (config->idle_disconnect) {
		/* Always attempt to disconnect, even if i2c_transfer fails */
		disconnect_res = tca954x_set_channel(down_cfg->root, 0);
		if (disconnect_res != 0 && res == 0) {
			res = disconnect_res;
		}
	}

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
		if (!gpio_is_ready_dt(&config->reset_gpios)) {
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

	i2c_tca954x->selected_chan = config->por_chan;

	if (config->idle_disconnect) {
		int res = tca954x_set_channel(dev, 0);

		if (res != 0) {
			LOG_ERR("%s: failed to disconnect channels", dev->name);
			return res;
		}
	}

	return 0;
}

static int tca954x_channel_init(const struct device *dev)
{
	const struct tca954x_channel_config *chan_cfg = dev->config;

	if (!device_is_ready(chan_cfg->root)) {
		LOG_ERR("I2C mux root %s not ready", chan_cfg->root->name);
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(i2c, tca954x_api_funcs) = {
	.configure = tca954x_configure,
	.transfer = tca954x_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

BUILD_ASSERT(CONFIG_I2C_TCA954X_CHANNEL_INIT_PRIO > CONFIG_I2C_TCA954X_ROOT_INIT_PRIO,
	     "I2C multiplexer channels must be initialized after their root");

#define TCA954x_CHILD_DEFINE(node_id, name, nchans, en_mask)			\
	BUILD_ASSERT(DT_REG_ADDR(node_id) < (nchans),				\
		     "Invalid channel reg for " DT_NODE_FULL_NAME(node_id));	\
	static const struct tca954x_channel_config				\
		name##_down_config_##node_id = {				\
		.chan_mask = (en_mask) ? (en_mask) | DT_REG_ADDR(node_id)	\
					       : BIT(DT_REG_ADDR(node_id)),	\
		.root = DEVICE_DT_GET(DT_PARENT(node_id)),			\
	};									\
	DEVICE_DT_DEFINE(node_id,						\
			 tca954x_channel_init,					\
			 NULL,							\
			 NULL,							\
			 &name##_down_config_##node_id,				\
			 POST_KERNEL, CONFIG_I2C_TCA954X_CHANNEL_INIT_PRIO,	\
			 &tca954x_api_funcs);

/*
 * name:    token used to build unique symbol names
 * nchans:  number of downstream channels
 * en_mask: channel enable bit for mux variants (one channel at a time,
 *          selected by number), 0 for switch variants (channel bitmask)
 * por:     control register value after power-on or reset
 */
#define TCA954x_ROOT_DEFINE(name, inst, nchans, en_mask, por)				\
	static const struct tca954x_root_config name##_cfg_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.por_chan = por,							\
		.reset_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		.idle_disconnect = DT_INST_PROP(inst, i2c_mux_idle_disconnect),		\
	};										\
	static struct tca954x_root_data name##_data_##inst = {				\
		.lock = Z_MUTEX_INITIALIZER(name##_data_##inst.lock),			\
	};										\
	I2C_DEVICE_DT_INST_DEFINE(inst,							\
				  tca954x_root_init, NULL,				\
				  &name##_data_##inst, &name##_cfg_##inst,		\
				  POST_KERNEL, CONFIG_I2C_TCA954X_ROOT_INIT_PRIO,	\
				  NULL);						\
	DT_INST_FOREACH_CHILD_VARGS(inst, TCA954x_CHILD_DEFINE, name, nchans, en_mask);

/*
 * TCA9544A: 4 channels mux
 */
#define TCA9544A_INIT(n) TCA954x_ROOT_DEFINE(tca9544a, n, 4, BIT(2), 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tca9544a
DT_INST_FOREACH_STATUS_OKAY(TCA9544A_INIT)

/*
 * TCA9546A: 4 channels switch
 */
#define TCA9546A_INIT(n) TCA954x_ROOT_DEFINE(tca9546a, n, 4, 0, 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tca9546a
DT_INST_FOREACH_STATUS_OKAY(TCA9546A_INIT)

/*
 * TCA9548A: 8 channels switch
 */
#define TCA9548A_INIT(n) TCA954x_ROOT_DEFINE(tca9548a, n, 8, 0, 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tca9548a
DT_INST_FOREACH_STATUS_OKAY(TCA9548A_INIT)

/*
 * PCA9547: 8 channels mux, powers up with channel 0 connected
 */
#define PCA9547_INIT(n) TCA954x_ROOT_DEFINE(pca9547, n, 8, BIT(3), BIT(3) | 0)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_pca9547
DT_INST_FOREACH_STATUS_OKAY(PCA9547_INIT)
