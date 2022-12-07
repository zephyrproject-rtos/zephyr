/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(main);

struct i2c_virtual_data {
	sys_slist_t targets;
};

int i2c_virtual_runtime_configure(const struct device *dev, uint32_t config)
{
	return 0;
}

static struct i2c_target_config *find_address(struct i2c_virtual_data *data,
					     uint16_t address, bool is_10bit)
{
	struct i2c_target_config *cfg = NULL;
	sys_snode_t *node;
	bool search_10bit;

	SYS_SLIST_FOR_EACH_NODE(&data->targets, node) {
		cfg = CONTAINER_OF(node, struct i2c_target_config, node);

		search_10bit = (cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS);

		if (cfg->address == address && search_10bit == is_10bit) {
			return cfg;
		}
	}

	return NULL;
}

/* Attach I2C targets */
int i2c_virtual_target_register(const struct device *dev,
			       struct i2c_target_config *config)
{
	struct i2c_virtual_data *data = dev->data;

	if (!config) {
		return -EINVAL;
	}

	/* Check the address is unique */
	if (find_address(data, config->address,
			 (config->flags & I2C_TARGET_FLAGS_ADDR_10_BITS))) {
		return -EINVAL;
	}

	sys_slist_append(&data->targets, &config->node);

	return 0;
}


int i2c_virtual_target_unregister(const struct device *dev,
				 struct i2c_target_config *config)
{
	struct i2c_virtual_data *data = dev->data;

	if (!config) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&data->targets, &config->node)) {
		return -EINVAL;
	}

	return 0;
}

static int i2c_virtual_msg_write(const struct device *dev,
				 struct i2c_msg *msg,
				 struct i2c_target_config *config,
				 bool prev_write)
{
	unsigned int len = 0U;
	uint8_t *buf = msg->buf;
	int ret;

	if (!prev_write) {
		config->callbacks->write_requested(config);
	}

	len = msg->len;
	while (len) {

		ret = config->callbacks->write_received(config, *buf);
		if (ret) {
			goto error;
		}
		buf++;
		len--;
	}

	if (!(msg->flags & I2C_MSG_RESTART) && msg->flags & I2C_MSG_STOP) {
		config->callbacks->stop(config);
	}

	return 0;
error:
	LOG_DBG("%s: NACK", __func__);

	return -EIO;
}

static int i2c_virtual_msg_read(const struct device *dev, struct i2c_msg *msg,
				struct i2c_target_config *config)
{
	unsigned int len = msg->len;
	uint8_t *buf = msg->buf;

	if (!msg->len) {
		return 0;
	}

	config->callbacks->read_requested(config, buf);
	buf++;
	len--;

	while (len) {
		config->callbacks->read_processed(config, buf);
		buf++;
		len--;
	}

	if (!(msg->flags & I2C_MSG_RESTART) && msg->flags & I2C_MSG_STOP) {
		config->callbacks->stop(config);
	}

	return 0;
}

#define OPERATION(msg) (((struct i2c_msg *) msg)->flags & I2C_MSG_RW_MASK)

static int i2c_virtual_transfer(const struct device *dev, struct i2c_msg *msg,
				uint8_t num_msgs, uint16_t target)
{
	struct i2c_virtual_data *data = dev->data;
	struct i2c_msg *current, *next;
	struct i2c_target_config *cfg;
	bool is_write = false;
	int ret = 0;

	cfg = find_address(data, target, (msg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS));
	if (!cfg) {
		return -EIO;
	}

	current = msg;
	current->flags |= I2C_MSG_RESTART;
	while (num_msgs > 0) {
		if (num_msgs > 1) {
			next = current + 1;

			/*
			 * Stop or restart condition between messages
			 * of different directions is required
			 */
			if (OPERATION(current) != OPERATION(next)) {
				if (!(next->flags & I2C_MSG_RESTART)) {
					ret = -EINVAL;
					break;
				}
			}
		}

		/* Stop condition is required for the last message */
		if ((num_msgs == 1U) && !(current->flags & I2C_MSG_STOP)) {
			ret = -EINVAL;
			break;
		}

		if ((current->flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = i2c_virtual_msg_write(dev, current,
						    cfg, is_write);
			is_write = true;
		} else {
			ret = i2c_virtual_msg_read(dev, current, cfg);
			is_write = false;
		}

		if (ret < 0) {
			break;
		}

		current++;
		num_msgs--;
	}

	return ret;
}

static const struct i2c_driver_api api_funcs = {
	.configure = i2c_virtual_runtime_configure,
	.transfer = i2c_virtual_transfer,
	.target_register = i2c_virtual_target_register,
	.target_unregister = i2c_virtual_target_unregister,
};

static int i2c_virtual_init(const struct device *dev)
{
	struct i2c_virtual_data *data = dev->data;

	sys_slist_init(&data->targets);

	return 0;
}

static struct i2c_virtual_data i2c_virtual_dev_data_0;

DEVICE_DEFINE(i2c_virtual_0, CONFIG_I2C_VIRTUAL_NAME, &i2c_virtual_init,
		NULL, &i2c_virtual_dev_data_0, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&api_funcs);
