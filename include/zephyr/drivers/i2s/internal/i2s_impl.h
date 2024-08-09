/*
 * Copyright (c) 2017 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for I2S APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2S_H_
#error "Should only be included by zephyr/drivers/i2s.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2S_INTERNAL_I2S_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2S_INTERNAL_I2S_IMPL_H_

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_i2s_configure(const struct device *dev,
				       enum i2s_dir dir,
				       const struct i2s_config *cfg)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->configure(dev, dir, cfg);
}

static inline int z_impl_i2s_trigger(const struct device *dev,
				     enum i2s_dir dir,
				     enum i2s_trigger_cmd cmd)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->trigger(dev, dir, cmd);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2S_INTERNAL_I2S_IMPL_H_ */
