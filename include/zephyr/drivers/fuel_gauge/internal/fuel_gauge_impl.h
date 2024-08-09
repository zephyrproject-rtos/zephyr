/*
 * Copyright 2022 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BATTERY_H_
#error "Should only be included by zephyr/drivers/fuel_gauge.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_FUEL_GAUGE_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_FUEL_GAUGE_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

static inline int z_impl_fuel_gauge_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
					     union fuel_gauge_prop_val *val)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->get_property == NULL) {
		return -ENOSYS;
	}

	return api->get_property(dev, prop, val);
}

static inline int z_impl_fuel_gauge_get_props(const struct device *dev,
					      fuel_gauge_prop_t *props,
					      union fuel_gauge_prop_val *vals, size_t len)
{
	const struct fuel_gauge_driver_api *api = dev->api;

	for (int i = 0; i < len; i++) {
		int ret = api->get_property(dev, props[i], vals + i);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

static inline int z_impl_fuel_gauge_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
					     union fuel_gauge_prop_val val)
{
	const struct fuel_gauge_driver_api *api = dev->api;

	if (api->set_property == NULL) {
		return -ENOSYS;
	}

	return api->set_property(dev, prop, val);
}

static inline int z_impl_fuel_gauge_set_props(const struct device *dev,
					      fuel_gauge_prop_t *props,
					      union fuel_gauge_prop_val *vals, size_t len)
{
	for (int i = 0; i < len; i++) {
		int ret = fuel_gauge_set_prop(dev, props[i], vals[i]);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

static inline int z_impl_fuel_gauge_get_buffer_prop(const struct device *dev,
						   fuel_gauge_prop_t prop_type,
						   void *dst, size_t dst_len)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->get_buffer_property == NULL) {
		return -ENOSYS;
	}

	return api->get_buffer_property(dev, prop_type, dst, dst_len);
}

static inline int z_impl_fuel_gauge_battery_cutoff(const struct device *dev)
{
	const struct fuel_gauge_driver_api *api = (const struct fuel_gauge_driver_api *)dev->api;

	if (api->battery_cutoff == NULL) {
		return -ENOSYS;
	}

	return api->battery_cutoff(dev);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_FUEL_GAUGE_IMPL_H_ */
