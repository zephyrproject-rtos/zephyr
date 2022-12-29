/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/fuel_gauge.h>

static inline int z_vrfy_fuel_gauge_get_prop(const struct device *dev,
					     struct fuel_gauge_get_property *props,
					     size_t props_len)
{
	struct fuel_gauge_get_property k_props[props_len];

	Z_OOPS(Z_SYSCALL_DRIVER_FUEL_GAUGE(dev, get_property));

	Z_OOPS(z_user_from_copy(k_props, props,
				props_len * sizeof(struct fuel_gauge_get_property)));

	int ret = z_impl_fuel_gauge_get_prop(dev, k_props, props_len);

	Z_OOPS(z_user_to_copy(props, k_props, props_len * sizeof(struct fuel_gauge_get_property)));

	return ret;
}

#include <syscalls/fuel_gauge_get_prop_mrsh.c>

static inline int z_vrfy_fuel_gauge_set_prop(const struct device *dev,
					     struct fuel_gauge_set_property *props,
					     size_t props_len)
{
	struct fuel_gauge_set_property k_props[props_len];

	Z_OOPS(Z_SYSCALL_DRIVER_FUEL_GAUGE(dev, set_property));

	Z_OOPS(z_user_from_copy(k_props, props,
				props_len * sizeof(struct fuel_gauge_set_property)));

	int ret = z_impl_fuel_gauge_set_prop(dev, k_props, props_len);

	Z_OOPS(z_user_to_copy(props, k_props, props_len * sizeof(struct fuel_gauge_set_property)));

	return ret;
}

#include <syscalls/fuel_gauge_set_prop_mrsh.c>
