/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Backend APIs for the fuel gauge emulators.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_EMUL_FUEL_GAUGE_H_
#define ZEPHYR_INCLUDE_DRIVERS_EMUL_FUEL_GAUGE_H_

#include <stdint.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/fuel_gauge.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fuel gauge backend emulator APIs
 * @defgroup fuel_gauge_emulator_backend Fuel gauge backend emulator APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */
__subsystem struct fuel_gauge_emul_driver_api {
	int (*set_battery_charging)(const struct emul *emul, uint32_t uV, int uA);
};
/**
 * @endcond
 */

/**
 * @brief Set charging for fuel gauge associated battery.
 *
 * Set how much the battery associated with a fuel gauge IC is charging or discharging. Where
 * voltage is always positive and a positive or negative current denotes charging or discharging,
 * respectively.
 *
 * @param target Pointer to the emulator structure for the fuel gauge emulator instance.
 * @param uV Microvolts describing the battery voltage.
 * @param uA Microamps describing the battery current where negative is discharging.
 *
 * @retval 0 If successful.
 * @retval -EINVAL if mV or mA are 0.
 */
static inline int emul_fuel_gauge_set_battery_charging(const struct emul *target, uint32_t uV,
						       int uA)
{
	const struct fuel_gauge_emul_driver_api *backend_api =
		(const struct fuel_gauge_emul_driver_api *)target->backend_api;

	return backend_api->set_battery_charging(target, uV, uA);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_EMUL_FUEL_GAUGE_H_*/
