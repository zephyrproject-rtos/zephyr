/*
 * Copyright (c) 2024-2026 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2026 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for Morse drivers
 */

#ifndef ZEPHYR_INCLUDE_MORSE_DEVICE_H_
#define ZEPHYR_INCLUDE_MORSE_DEVICE_H_

/**
 * @brief Morse Interface
 * @defgroup morse_interface Morse Interface
 * @since 4.4
 * @version 1.0.0
 * @ingroup morse_code
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Morse Code bit state. */
enum morse_bit_state {
	/** Off */
	MORSE_BIT_STATE_OFF,
	/** On */
	MORSE_BIT_STATE_ON,
};

/**
 * @typedef morse_bit_state_cb_t
 * @brief Define the Morse bit state callback function signature for
 * morse_set_rx_callback() function.
 *
 * @param dev   Morse device instance.
 * @param state The bit state On/Off.
 * @param morse The morse context.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the requested action is not supported.
 */
typedef int (*morse_bit_state_cb_t)(const struct device *dev,
				    enum morse_bit_state state,
				    const struct device *morse);

/**
 * @brief Set the tx bit state in the wire.
 *
 * @param dev   Morse device instance.
 * @param state The bit state On/Off.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 */
__syscall int morse_set_tx_bit_state(const struct device *dev,
				     enum morse_bit_state state);

/**
 * @brief Set the Morse bit state receive callback.
 *
 * @param dev      Morse device instance.
 * @param callback The bit state callback to register.
 * @param morse    The morse context passed to the callback.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 * @retval -EINVAL If @p callback or @p morse is NULL.
 */
static inline int morse_set_rx_callback(const struct device *dev,
					morse_bit_state_cb_t callback,
					const struct device *morse);

/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @def_driverbackendgroup{Morse,morse_interface}
 * @{
 */

/**
 * @brief Set the tx bit state in the wire.
 * See morse_set_tx_bit_state() for argument description.
 */
typedef int (*morse_set_tx_bit_state_t)(const struct device *dev,
					enum morse_bit_state state);

/**
 * @brief Set the Morse bit state receive callback.
 * See morse_set_rx_callback() for argument description.
 */
typedef int (*morse_set_rx_callback_t)(const struct device *dev,
				       morse_bit_state_cb_t callback,
				       const struct device *morse);

/**
 * @driver_ops{Morse}
 */
__subsystem struct morse_driver_api {
	/** @driver_ops_mandatory @copybrief morse_set_tx_bit_state */
	morse_set_tx_bit_state_t tx_bit_state;
	/** @driver_ops_optional @copybrief morse_set_rx_callback */
	morse_set_rx_callback_t rx_cb;
};

/**
 * @}
 */

static inline int z_impl_morse_set_tx_bit_state(const struct device *dev,
						enum morse_bit_state state)
{
	const struct morse_driver_api *api = (const struct morse_driver_api *)dev->api;

	if (api->tx_bit_state == NULL) {
		return -ENOSYS;
	}

	return api->tx_bit_state(dev, state);
}

static inline int morse_set_rx_callback(const struct device *dev,
					morse_bit_state_cb_t callback,
					const struct device *morse)
{
	const struct morse_driver_api *api = (const struct morse_driver_api *)dev->api;

	if (api->rx_cb == NULL) {
		return -ENOSYS;
	}

	if (callback == NULL || morse == NULL) {
		return -EINVAL;
	}

	return api->rx_cb(dev, callback, morse);
}

/** @endcond */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/morse_device.h>

#endif /* ZEPHYR_INCLUDE_MORSE_DEVICE_H_ */
