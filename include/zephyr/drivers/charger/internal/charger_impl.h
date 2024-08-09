/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for Charger APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CHARGER_H_
#error "Should only be included by zephyr/drivers/charger.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_CHARGER_INTERNAL_CHARGER_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CHARGER_INTERNAL_CHARGER_IMPL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline int z_impl_charger_get_prop(const struct device *dev, const charger_prop_t prop,
					  union charger_propval *val)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->get_property(dev, prop, val);
}

static inline int z_impl_charger_set_prop(const struct device *dev, const charger_prop_t prop,
					  const union charger_propval *val)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->set_property(dev, prop, val);
}

static inline int z_impl_charger_charge_enable(const struct device *dev, const bool enable)
{
	const struct charger_driver_api *api = (const struct charger_driver_api *)dev->api;

	return api->charge_enable(dev, enable);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CHARGER_INTERNAL_CHARGER_IMPL_H_ */
