/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
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
 * @since 4.1
 * @version 1.0.0
 * @ingroup io_interfaces
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
 * @param ctx   The morse context.
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
 * @retval -ENOTSUP If the requested action is not supported.
 */
__syscall int morse_set_tx_bit_state(const struct device *dev,
				     enum morse_bit_state state);

/**
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/** @brief Driver API structure. */
__subsystem struct morse_driver_api {
	int (*tx_bit_state)(const struct device *dev,
			    enum morse_bit_state state);
	int (*rx_cb)(const struct device *dev,
		     morse_bit_state_cb_t callback,
		     const struct device *morse);
};

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
