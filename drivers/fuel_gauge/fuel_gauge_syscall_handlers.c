/*
 * Copyright 2023 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/fuel_gauge.h>

static inline int z_vrfy_fuel_gauge_get_prop(const struct device *dev, fuel_gauge_prop_t prop,
					     union fuel_gauge_prop_val *val)
{
	union fuel_gauge_prop_val k_val;

	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, get_property));

	K_OOPS(k_usermode_from_copy(&k_val, val, sizeof(union fuel_gauge_prop_val)));

	int ret = z_impl_fuel_gauge_get_prop(dev, prop, &k_val);

	K_OOPS(k_usermode_to_copy(val, &k_val, sizeof(union fuel_gauge_prop_val)));

	return ret;
}

#include <syscalls/fuel_gauge_get_prop_mrsh.c>

static inline int z_vrfy_fuel_gauge_get_props(const struct device *dev, fuel_gauge_prop_t *props,
					      union fuel_gauge_prop_val *vals, size_t len)
{
	union fuel_gauge_prop_val k_vals[len];
	fuel_gauge_prop_t k_props[len];

	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, get_property));

	K_OOPS(k_usermode_from_copy(k_vals, vals, len * sizeof(union fuel_gauge_prop_val)));
	K_OOPS(k_usermode_from_copy(k_props, props, len * sizeof(fuel_gauge_prop_t)));

	int ret = z_impl_fuel_gauge_get_props(dev, k_props, k_vals, len);

	K_OOPS(k_usermode_to_copy(vals, k_vals, len * sizeof(union fuel_gauge_prop_val)));

	return ret;
}

#include <syscalls/fuel_gauge_get_props_mrsh.c>

static inline int z_vrfy_fuel_gauge_set_prop(const struct device *dev, fuel_gauge_prop_t prop,
					     union fuel_gauge_prop_val val)
{
	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, set_property));

	int ret = z_impl_fuel_gauge_set_prop(dev, prop, val);

	return ret;
}

#include <syscalls/fuel_gauge_set_prop_mrsh.c>

static inline int z_vrfy_fuel_gauge_set_props(const struct device *dev, fuel_gauge_prop_t *props,
					      union fuel_gauge_prop_val *vals, size_t len)
{
	union fuel_gauge_prop_val k_vals[len];
	fuel_gauge_prop_t k_props[len];

	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, set_property));

	K_OOPS(k_usermode_from_copy(k_vals, vals, len * sizeof(union fuel_gauge_prop_val)));
	K_OOPS(k_usermode_from_copy(k_props, props, len * sizeof(fuel_gauge_prop_t)));

	int ret = z_impl_fuel_gauge_set_props(dev, k_props, k_vals, len);

	/* We only copy back vals because props will never be modified */
	K_OOPS(k_usermode_to_copy(vals, k_vals, len * sizeof(union fuel_gauge_prop_val)));

	return ret;
}

#include <syscalls/fuel_gauge_set_props_mrsh.c>

static inline int z_vrfy_fuel_gauge_get_buffer_prop(const struct device *dev,
						    fuel_gauge_prop_t prop, void *dst,
						    size_t dst_len)
{
	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, get_buffer_property));

	K_OOPS(K_SYSCALL_MEMORY_WRITE(dst, dst_len));

	int ret = z_impl_fuel_gauge_get_buffer_prop(dev, prop, dst, dst_len);

	return ret;
}

#include <syscalls/fuel_gauge_get_buffer_prop_mrsh.c>

static inline int z_vrfy_fuel_gauge_battery_cutoff(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_FUEL_GAUGE(dev, battery_cutoff));

	return z_impl_fuel_gauge_battery_cutoff(dev);
}

#include <syscalls/fuel_gauge_battery_cutoff_mrsh.c>
