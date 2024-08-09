/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for 1-Wire Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_W1_H_
#error "Should only be included by zephyr/drivers/w1.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_W1_INTERNAL_W1_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_W1_INTERNAL_W1_IMPL_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_w1_change_bus_lock(const struct device *dev, bool lock)
{
	struct w1_master_data *ctrl_data = (struct w1_master_data *)dev->data;
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	if (api->change_bus_lock) {
		return api->change_bus_lock(dev, lock);
	}

	if (lock) {
		return k_mutex_lock(&ctrl_data->bus_lock, K_FOREVER);
	} else {
		return k_mutex_unlock(&ctrl_data->bus_lock);
	}
}

static inline int z_impl_w1_reset_bus(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->reset_bus(dev);
}

static inline int z_impl_w1_read_bit(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_bit(dev);
}

static inline int z_impl_w1_write_bit(const struct device *dev, bool bit)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_bit(dev, bit);
}

static inline int z_impl_w1_read_byte(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_byte(dev);
}

static inline int z_impl_w1_write_byte(const struct device *dev, uint8_t byte)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_byte(dev, byte);
}

static inline size_t z_impl_w1_get_slave_count(const struct device *dev)
{
	const struct w1_master_config *ctrl_cfg =
		(const struct w1_master_config *)dev->config;

	return ctrl_cfg->slave_count;
}

static inline int z_impl_w1_configure(const struct device *dev,
				      enum w1_settings_type type, uint32_t value)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->configure(dev, type, value);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_W1_INTERNAL_W1_IMPL_H_ */
