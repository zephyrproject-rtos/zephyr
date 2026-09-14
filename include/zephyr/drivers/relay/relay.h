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
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 *
 * The relay API exposes a hardware-agnostic on/off view of a single relay. A
 * backend driver (for example the PWM or GPIO relay) implements
 * @ref relay_driver_api and hides all coil-drive details behind it.
 */

#include <errno.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Relay output state. */
enum relay_state {
	RELAY_STATE_OFF, /* Relay is inactive / de-energized */
	RELAY_STATE_ON,  /* Relay is active / energized */
};

/** @brief Set the on/off state of the relay. */
typedef int (*relay_set_state_t)(const struct device *dev, enum relay_state state);

/** @brief Read back the on/off state of the relay. */
typedef int (*relay_get_state_t)(const struct device *dev, enum relay_state *state);

/** @brief Relay driver class API. */
__subsystem struct relay_driver_api {
	relay_set_state_t set_state; /* Set the logical state (active/inactive) of the relay */
	relay_get_state_t get_state; /* Get the current logical state of the relay */
};

/**
 * @brief Set the on/off state of the relay.
 *
 * @param dev Relay device instance.
 * @param state Target relay state (@ref RELAY_STATE_ON / @ref RELAY_STATE_OFF).
 * @retval 0 On success.
 * @retval -ENOSYS If the backend does not implement state control.
 * @retval -errno Negative errno from the backend.
 */
__syscall int relay_set_state(const struct device *dev, enum relay_state state);

static inline int z_impl_relay_set_state(const struct device *dev, enum relay_state state)
{
	const struct relay_driver_api *api = DEVICE_API_GET(relay, dev);

	if (api->set_state == NULL) {
		return -ENOSYS;
	}
	return api->set_state(dev, state);
}

/**
 * @brief Read back the last requested on/off state of the relay.
 *
 * @param dev Relay device instance.
 * @param state Output: current relay state.
 * @retval 0 On success.
 * @retval -ENOSYS If the backend does not implement state read-back.
 */
__syscall int relay_get_state(const struct device *dev, enum relay_state *state);

static inline int z_impl_relay_get_state(const struct device *dev, enum relay_state *state)
{
	const struct relay_driver_api *api = DEVICE_API_GET(relay, dev);

	if (api->get_state == NULL) {
		return -ENOSYS;
	}
	return api->get_state(dev, state);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/relay.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RELAY_RELAY_H_ */
