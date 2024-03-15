/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/charger.h>

static inline int z_vrfy_charger_get_prop(const struct device *dev, const charger_prop_t prop,
					  union charger_propval *val)
{
	union charger_propval k_val;

	K_OOPS(K_SYSCALL_DRIVER_CHARGER(dev, get_property));

	int ret = z_impl_charger_get_prop(dev, prop, &k_val);

	K_OOPS(k_usermode_to_copy(val, &k_val, sizeof(union charger_propval)));

	return ret;
}

#include <syscalls/charger_get_prop_mrsh.c>

static inline int z_vrfy_charger_set_prop(const struct device *dev, const charger_prop_t prop,
					  const union charger_propval *val)
{
	union charger_propval k_val;

	K_OOPS(K_SYSCALL_DRIVER_CHARGER(dev, set_property));

	K_OOPS(k_usermode_from_copy(&k_val, val, sizeof(union charger_propval)));

	return z_impl_charger_set_prop(dev, prop, &k_val);
}

#include <syscalls/charger_set_prop_mrsh.c>

static inline int z_vrfy_charger_charge_enable(const struct device *dev, const bool enable)
{
	K_OOPS(K_SYSCALL_DRIVER_CHARGER(dev, charge_enable));

	return z_impl_charger_charge_enable(dev, enable);
}

#include <syscalls/charger_charge_enable_mrsh.c>
