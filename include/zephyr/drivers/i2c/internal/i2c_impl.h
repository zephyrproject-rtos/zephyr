/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementations for I2C APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_H_
#error "Should only be included by zephyr/drivers/i2c.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_INTERNAL_I2C_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_INTERNAL_I2C_IMPL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_i2c_configure(const struct device *dev,
				       uint32_t dev_config)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	return api->configure(dev, dev_config);
}

static inline int z_impl_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_driver_api *api = (const struct i2c_driver_api *)dev->api;

	if (api->get_config == NULL) {
		return -ENOSYS;
	}

	return api->get_config(dev, dev_config);
}

static inline int z_impl_i2c_transfer(const struct device *dev,
				      struct i2c_msg *msgs, uint8_t num_msgs,
				      uint16_t addr)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	if (!num_msgs) {
		return 0;
	}

	msgs[num_msgs - 1].flags |= I2C_MSG_STOP;

	int res =  api->transfer(dev, msgs, num_msgs, addr);

	i2c_xfer_stats(dev, msgs, num_msgs);

	if (IS_ENABLED(CONFIG_I2C_DUMP_MESSAGES)) {
		i2c_dump_msgs_rw(dev, msgs, num_msgs, addr, true);
	}

	return res;
}

static inline int z_impl_i2c_recover_bus(const struct device *dev)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	if (api->recover_bus == NULL) {
		return -ENOSYS;
	}

	return api->recover_bus(dev);
}

static inline int z_impl_i2c_target_driver_register(const struct device *dev)
{
	const struct i2c_target_driver_api *api =
		(const struct i2c_target_driver_api *)dev->api;

	return api->driver_register(dev);
}

static inline int z_impl_i2c_target_driver_unregister(const struct device *dev)
{
	const struct i2c_target_driver_api *api =
		(const struct i2c_target_driver_api *)dev->api;

	return api->driver_unregister(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_INTERNAL_I2C_IMPL_H_ */
