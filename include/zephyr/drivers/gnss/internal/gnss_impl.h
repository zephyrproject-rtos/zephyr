/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file gnss.h
 *
 * @brief Inline syscall implementations for GNSS APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_H_
#error "Should only be included by zephyr/drivers/gnss.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_GNSS_INTERNAL_GNSS_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_GNSS_INTERNAL_GNSS_IMPL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/data/navigation.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_gnss_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_fix_rate == NULL) {
		return -ENOSYS;
	}

	return api->set_fix_rate(dev, fix_interval_ms);
}

static inline int z_impl_gnss_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_fix_rate == NULL) {
		return -ENOSYS;
	}

	return api->get_fix_rate(dev, fix_interval_ms);
}

static inline int z_impl_gnss_set_periodic_config(const struct device *dev,
						  const struct gnss_periodic_config *config)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_periodic_config == NULL) {
		return -ENOSYS;
	}

	return api->set_periodic_config(dev, config);
}

static inline int z_impl_gnss_get_periodic_config(const struct device *dev,
						  struct gnss_periodic_config *config)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_periodic_config == NULL) {
		return -ENOSYS;
	}

	return api->get_periodic_config(dev, config);
}

static inline int z_impl_gnss_set_navigation_mode(const struct device *dev,
						  enum gnss_navigation_mode mode)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_navigation_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_navigation_mode(dev, mode);
}

static inline int z_impl_gnss_get_navigation_mode(const struct device *dev,
						  enum gnss_navigation_mode *mode)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_navigation_mode == NULL) {
		return -ENOSYS;
	}

	return api->get_navigation_mode(dev, mode);
}

static inline int z_impl_gnss_set_enabled_systems(const struct device *dev,
						  gnss_systems_t systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->set_enabled_systems == NULL) {
		return -ENOSYS;
	}

	return api->set_enabled_systems(dev, systems);
}

static inline int z_impl_gnss_get_enabled_systems(const struct device *dev,
						  gnss_systems_t *systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_enabled_systems == NULL) {
		return -ENOSYS;
	}

	return api->get_enabled_systems(dev, systems);
}

static inline int z_impl_gnss_get_supported_systems(const struct device *dev,
						    gnss_systems_t *systems)
{
	const struct gnss_driver_api *api = (const struct gnss_driver_api *)dev->api;

	if (api->get_supported_systems == NULL) {
		return -ENOSYS;
	}

	return api->get_supported_systems(dev, systems);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GNSS_INTERNAL_GNSS_IMPL_H_ */
