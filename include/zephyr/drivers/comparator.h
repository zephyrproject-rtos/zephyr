/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_

/**
 * @file
 * @ingroup comparator_interface
 * @brief Main header file for comparator driver API.
 */

/**
 * @brief Interfaces for comparators.
 * @defgroup comparator_interface Comparator
 * @since 4.0
 * @version 0.8.0
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

/**
 * @brief Comparator callback template
 *
 * @param dev Comparator device
 * @param user_data Pointer to the user data that was provided when the trigger callback was set
 */
typedef void (*comparator_callback_t)(const struct device *dev, void *user_data);

/**
 * @def_driverbackendgroup{Comparator,comparator_interface}
 * @{
 */

/**
 * @brief Get comparator's output state.
 * See comparator_get_output() for argument description.
 */
typedef int (*comparator_api_get_output)(const struct device *dev);

/**
 * @brief Set comparator's trigger.
 * See comparator_set_trigger() for argument description.
 */
typedef int (*comparator_api_set_trigger)(const struct device *dev,
					  enum comparator_trigger trigger);

/**
 * @brief Set comparator's trigger callback.
 * See comparator_set_trigger_callback() for argument description.
 */
typedef int (*comparator_api_set_trigger_callback)(const struct device *dev,
						   comparator_callback_t callback,
						   void *user_data);

/**
 * @brief Check if comparator's trigger is pending and clear it.
 * See comparator_trigger_is_pending() for argument description.
 */
typedef int (*comparator_api_trigger_is_pending)(const struct device *dev);

/**
 * @driver_ops{Comparator}
 */
__subsystem struct comparator_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief comparator_get_output
	 */
	comparator_api_get_output get_output;
	/**
	 * @driver_ops_mandatory @copybrief comparator_set_trigger
	 */
	comparator_api_set_trigger set_trigger;
	/**
	 * @driver_ops_mandatory @copybrief comparator_set_trigger_callback
	 */
	comparator_api_set_trigger_callback set_trigger_callback;
	/**
	 * @driver_ops_mandatory @copybrief comparator_trigger_is_pending
	 */
	comparator_api_trigger_is_pending trigger_is_pending;
};

/** @} */

/**
 * @brief Get comparator's output state
 *
 * @param dev Comparator device
 *
 * @retval 1 Output state is high.
 * @retval 0 Output state is low.
 * @return Negative errno value on failure.
 */
__syscall int comparator_get_output(const struct device *dev);

static inline int z_impl_comparator_get_output(const struct device *dev)
{
	return DEVICE_API_GET(comparator, dev)->get_output(dev);
}

/**
 * @brief Set comparator's trigger
 *
 * @param dev Comparator device
 * @param trigger Trigger for signal and callback
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int comparator_set_trigger(const struct device *dev,
				     enum comparator_trigger trigger);

static inline int z_impl_comparator_set_trigger(const struct device *dev,
						enum comparator_trigger trigger)
{
	return DEVICE_API_GET(comparator, dev)->set_trigger(dev, trigger);
}

/**
 * @brief Set comparator's trigger callback
 *
 * @param dev Comparator device
 * @param callback Trigger callback
 * @param user_data User data passed to callback
 *
 * @return 0 on success, negative errno value on failure.
 *
 * @note Set callback to NULL to disable callback
 * @note Callback is called immediately if trigger is pending
 */
static inline int comparator_set_trigger_callback(const struct device *dev,
						  comparator_callback_t callback,
						  void *user_data)
{
	return DEVICE_API_GET(comparator, dev)->set_trigger_callback(dev, callback, user_data);
}

/**
 * @brief Check if comparator's trigger is pending and clear it
 *
 * @param dev Comparator device
 *
 * @retval 1 Trigger was pending
 * @retval 0 Trigger was cleared
 * @return Negative errno value on failure.
 */
__syscall int comparator_trigger_is_pending(const struct device *dev);

static inline int z_impl_comparator_trigger_is_pending(const struct device *dev)
{
	return DEVICE_API_GET(comparator, dev)->trigger_is_pending(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/comparator.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_ */
