/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RELAY_RELAY_H_
#define ZEPHYR_INCLUDE_DRIVERS_RELAY_RELAY_H_

/**
 * @file
 * @brief Public API for binary relay output drivers.
 */

/**
 * @brief Relay Interface
 * @defgroup relay_interface Relay
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 *
 * The relay API exposes a hardware-agnostic on/off view of a single relay. A
 * backend driver (for example the PWM or GPIO relay) implements
 * @ref relay_driver_api and hides all coil-drive details behind it.
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the on/off state of the relay. */
typedef int (*relay_set_state_t)(const struct device *dev, bool on);

/** @brief Read back the on/off state of the relay. */
typedef int (*relay_get_state_t)(const struct device *dev, bool *on);

/** @brief Relay driver class API. */
__subsystem struct relay_driver_api {
	/** @driver_ops_mandatory @copybrief relay_set_state */
	relay_set_state_t set_state;
	/** @driver_ops_optional @copybrief relay_get_state */
	relay_get_state_t get_state;
};

/**
 * @brief Set the on/off state of the relay.
 *
 * @param dev Relay device instance.
 * @param on true to energize the coil, false to release it.
 * @retval 0 On success.
 * @retval -errno Negative errno from the backend.
 */
__syscall int relay_set_state(const struct device *dev, bool on);

static inline int z_impl_relay_set_state(const struct device *dev, bool on)
{
	const struct relay_driver_api *api = DEVICE_API_GET(relay, dev);

	return api->set_state(dev, on);
}

/**
 * @brief Read back the last requested on/off state of the relay.
 *
 * @param dev Relay device instance.
 * @param on Output: true if the coil is energized, false if released.
 * @retval 0 On success.
 * @retval -ENOSYS If the backend does not implement state read-back.
 * @retval -errno Other negative errno on failure.
 */
__syscall int relay_get_state(const struct device *dev, bool *on);

static inline int z_impl_relay_get_state(const struct device *dev, bool *on)
{
	const struct relay_driver_api *api = DEVICE_API_GET(relay, dev);

	if (api->get_state == NULL) {
		return -ENOSYS;
	}

	return api->get_state(dev, on);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/relay.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RELAY_RELAY_H_ */
