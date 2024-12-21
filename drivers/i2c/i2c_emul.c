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
#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_cfg;
#endif
};

struct i2c_emul_config {
	struct emul_list_for_bus emul_list;
	bool target_buffered_mode;
	const struct i2c_dt_spec *forward_list;
	uint16_t forward_list_size;
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

#ifdef CONFIG_I2C_TARGET
static int i2c_emul_send_to_target(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs)
{
	struct i2c_emul_data *data = dev->data;
	const struct i2c_target_callbacks *callbacks = data->target_cfg->callbacks;

#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	const struct i2c_emul_config *config = dev->config;

	if (config->target_buffered_mode) {
		for (uint8_t i = 0; i < num_msgs; ++i) {
			if (i2c_is_read_op(&msgs[i])) {
				uint8_t *ptr = NULL;
				uint32_t len;
				int rc =
					callbacks->buf_read_requested(data->target_cfg, &ptr, &len);

				if (rc != 0) {
					return rc;
				}
				if (len > msgs[i].len) {
					LOG_ERR("buf_read_requested returned too many bytes");
					return -ENOMEM;
				}
				memcpy(msgs[i].buf, ptr, len);
			} else {
				callbacks->buf_write_received(data->target_cfg, msgs[i].buf,
							      msgs[i].len);
			}
			if (i2c_is_stop_op(&msgs[i])) {
				int rc = callbacks->stop(data->target_cfg);

				if (rc != 0) {
					return rc;
				}
			}
		}
		return 0;
	}
#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

	for (uint8_t i = 0; i < num_msgs; ++i) {
		LOG_DBG("    msgs[%u].flags? 0x%02x", i, msgs[i].flags);
		if (i2c_is_read_op(&msgs[i])) {
			for (uint32_t j = 0; j < msgs[i].len; ++j) {
				int rc = 0;

				if (j == 0) {
					LOG_DBG("    Calling read_requested with data %p",
						(void *)&msgs[i].buf[j]);
					rc = callbacks->read_requested(data->target_cfg,
								       &msgs[i].buf[j]);
				} else {
					LOG_DBG("    Calling read_processed with data %p",
						(void *)&msgs[i].buf[j]);
					rc = callbacks->read_processed(data->target_cfg,
								       &msgs[i].buf[j]);
				}
				if (rc != 0) {
					return rc;
				}
			}
		} else {
			for (uint32_t j = 0; j < msgs[i].len; ++j) {
				int rc = 0;

				if (j == 0) {
					LOG_DBG("    Calling write_requested");
					rc = callbacks->write_requested(data->target_cfg);
				}
				if (rc != 0) {
					return rc;
				}
				LOG_DBG("    Calling write_received with data 0x%02x",
					msgs[i].buf[j]);
				rc = callbacks->write_received(data->target_cfg, msgs[i].buf[j]);
				if (rc != 0) {
					return rc;
				}
			}
		}
		if (i2c_is_stop_op(&msgs[i])) {
			int rc = callbacks->stop(data->target_cfg);

			if (rc != 0) {
				return rc;
			}
		}
	}
	return 0;
}
#endif /* CONFIG_I2C_TARGET*/

static int i2c_emul_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			     uint16_t addr)
{
	const struct i2c_emul_config *conf = dev->config;
	struct i2c_emul *emul;
	const struct i2c_emul_api *api;
	int ret;

	LOG_DBG("%s(dev=%p, addr=0x%02x)", __func__, (void *)dev, addr);
#ifdef CONFIG_I2C_TARGET
	struct i2c_emul_data *data = dev->data;

	/*
	 * First check if the bus is configured as a target, targets either listen to the address or
	 * ignore the messages. So if the address doesn't match, we're just going to bail.
	 */
	LOG_DBG("    has_target_cfg? %d", data->target_cfg != NULL);
	if (data->target_cfg != NULL) {
		LOG_DBG("    target_cfg->address? 0x%02x", data->target_cfg->address);
		if (data->target_cfg->address != addr) {
			return -EINVAL;
		}
		LOG_DBG("    forwarding to target");
		return i2c_emul_send_to_target(dev, msgs, num_msgs);
	}
#endif /* CONFIG_I2C_TARGET */

	/*
	 * We're not a target, but lets check if we need to forward this request before we start
	 * looking for a peripheral.
	 */
	for (uint16_t i = 0; i < conf->forward_list_size; ++i) {
		LOG_DBG("    Checking forward list [%u].addr? 0x%02x", i,
			conf->forward_list[i].addr);
		if (conf->forward_list[i].addr == addr) {
			/* We need to forward this request */
			return i2c_transfer(conf->forward_list[i].bus, msgs, num_msgs, addr);
		}
	}

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

#ifdef CONFIG_I2C_TARGET
static int i2c_emul_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_emul_data *data = dev->data;

	data->target_cfg = cfg;
	return 0;
}

static int i2c_emul_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_emul_data *data = dev->data;

	if (data->target_cfg != cfg) {
		return -EINVAL;
	}

	data->target_cfg = NULL;
	return 0;
}
#endif /* CONFIG_I2C_TARGET */

/* Device instantiation */

static DEVICE_API(i2c, i2c_emul_api) = {
	.configure = i2c_emul_configure,
	.get_config = i2c_emul_get_config,
	.transfer = i2c_emul_transfer,
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_emul_target_register,
	.target_unregister = i2c_emul_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define EMUL_LINK_AND_COMMA(node_id)                                                               \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
	},

#define EMUL_FORWARD_ITEM(node_id, prop, idx)                                                      \
	{                                                                                          \
		.bus = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),                       \
		.addr = DT_PHA_BY_IDX(node_id, prop, idx, addr),                                   \
	},

#define I2C_EMUL_INIT(n)                                                                           \
	static const struct emul_link_for_bus emuls_##n[] = {                                      \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};                \
	static const struct i2c_dt_spec emul_forward_list_##n[] = {                                \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, forwards),                                    \
			    (DT_INST_FOREACH_PROP_ELEM(n, forwards, EMUL_FORWARD_ITEM)), ())};     \
	static struct i2c_emul_config i2c_emul_cfg_##n = {                                         \
		.emul_list =                                                                       \
			{                                                                          \
				.children = emuls_##n,                                             \
				.num_children = ARRAY_SIZE(emuls_##n),                             \
			},                                                                         \
		.target_buffered_mode = DT_INST_PROP(n, target_buffered_mode),                     \
		.forward_list = emul_forward_list_##n,                                             \
		.forward_list_size = ARRAY_SIZE(emul_forward_list_##n),                            \
	};                                                                                         \
	static struct i2c_emul_data i2c_emul_data_##n = {                                          \
		.bitrate = DT_INST_PROP(n, clock_frequency),                                       \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_emul_init, NULL, &i2c_emul_data_##n, &i2c_emul_cfg_##n,   \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_emul_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_EMUL_INIT)
