/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for fuel gauge emulators APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EMUL_FUEL_GAUGE_H_
#error "Should only be included by zephyr/drivers/emul_fuel_gauge.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_EMUL_FUEL_GAUGE_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_EMUL_FUEL_GAUGE_IMPL_H_

#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/fuel_gauge.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_emul_fuel_gauge_set_battery_charging(const struct emul *target,
							      uint32_t uV, int uA)
{
	const struct fuel_gauge_emul_driver_api *backend_api =
		(const struct fuel_gauge_emul_driver_api *)target->backend_api;

	if (backend_api->set_battery_charging == 0) {
		return -ENOTSUP;
	}

	return backend_api->set_battery_charging(target, uV, uA);
}

static inline int z_impl_emul_fuel_gauge_is_battery_cutoff(const struct emul *target, bool *cutoff)
{
	const struct fuel_gauge_emul_driver_api *backend_api =
		(const struct fuel_gauge_emul_driver_api *)target->backend_api;

	if (backend_api->is_battery_cutoff == 0) {
		return -ENOTSUP;
	}
	return backend_api->is_battery_cutoff(target, cutoff);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_INTERNAL_EMUL_FUEL_GAUGE_IMPL_H_*/
