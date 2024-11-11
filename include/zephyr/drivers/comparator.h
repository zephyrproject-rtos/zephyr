/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_

/**
 * @brief Comparator Interface
 * @defgroup comparator_interface Comparator Interface
 * @since 4.0
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Comparator trigger enumerations */
enum comparator_trigger {
	/** No trigger */
	COMPARATOR_TRIGGER_NONE = 0,
	/** Trigger on rising edge of comparator output */
	COMPARATOR_TRIGGER_RISING_EDGE,
	/** Trigger on falling edge of comparator output */
	COMPARATOR_TRIGGER_FALLING_EDGE,
	/** Trigger on both edges of comparator output */
	COMPARATOR_TRIGGER_BOTH_EDGES
};

/** Comparator callback template */
typedef void (*comparator_callback_t)(const struct device *dev, void *user_data);

/** @cond INTERNAL_HIDDEN */

typedef int (*comparator_api_get_output)(const struct device *dev);
typedef int (*comparator_api_set_trigger)(const struct device *dev,
					  enum comparator_trigger trigger);
typedef int (*comparator_api_set_trigger_callback)(const struct device *dev,
						   comparator_callback_t callback,
						   void *user_data);
typedef int (*comparator_api_trigger_is_pending)(const struct device *dev);

__subsystem struct comparator_driver_api {
	comparator_api_get_output get_output;
	comparator_api_set_trigger set_trigger;
	comparator_api_set_trigger_callback set_trigger_callback;
	comparator_api_trigger_is_pending trigger_is_pending;
};

/** @endcond */

/**
 * @brief Get comparator's output state
 *
 * @param dev Comparator device
 *
 * @retval 1 Output state is high
 * @retval 0 Output state is low
 * @retval -errno code Failure
 */
__syscall int comparator_get_output(const struct device *dev);

static inline int z_impl_comparator_get_output(const struct device *dev)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->get_output(dev);
}

/**
 * @brief Set comparator's trigger
 *
 * @param dev Comparator device
 * @param trigger Trigger for signal and callback
 *
 * @retval 0 Successful
 * @retval -errno code Failure
 */
__syscall int comparator_set_trigger(const struct device *dev,
				     enum comparator_trigger trigger);

static inline int z_impl_comparator_set_trigger(const struct device *dev,
						enum comparator_trigger trigger)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->set_trigger(dev, trigger);
}

/**
 * @brief Set comparator's trigger callback
 *
 * @param dev Comparator device
 * @param callback Trigger callback
 * @param user_data User data passed to callback
 *
 * @retval 0 Successful
 * @retval -errno code Failure
 *
 * @note Set callback to NULL to disable callback
 * @note Callback is called immediately if trigger is pending
 */
static inline int comparator_set_trigger_callback(const struct device *dev,
						  comparator_callback_t callback,
						  void *user_data)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->set_trigger_callback(dev, callback, user_data);
}

/**
 * @brief Check if comparator's trigger is pending and clear it
 *
 * @param dev Comparator device
 *
 * @retval 1 Trigger was pending
 * @retval 0 Trigger was cleared
 * @retval -errno code Failure
 */
__syscall int comparator_trigger_is_pending(const struct device *dev);

static inline int z_impl_comparator_trigger_is_pending(const struct device *dev)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->trigger_is_pending(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/comparator.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_ */
