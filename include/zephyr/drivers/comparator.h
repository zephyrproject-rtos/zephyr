/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_

/**
 * @brief Comparator Driver APIs
 * @defgroup comparator_interface Comparator Driver APIs
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/dt-bindings/comparator/comparator.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Comparator state flags
 * @{
 */

/** Positive input voltage is below the negative input voltage. */
#define COMPARATOR_STATE_BELOW BIT(0)
/** Positive input voltage is above the negative input voltage. */
#define COMPARATOR_STATE_ABOVE BIT(1)

/** @cond INTERNAL_HIDDEN */
#define COMPARATOR_STATE_RESULT_MASK  0x3UL
/* Position in the state bitfield from which vendor-specific flags can be defined. */
#define COMPARATOR_STATE_VENDOR_POS 8
/** @endcond */

/** @} */

/**
 * @brief Comparator callback.
 *
 * @param dev Comparator device instance.
 * @param state Bitfield that specifies the reported state. This parameter can
 *              contain COMPARATOR_STATE_* values and vendor-specific flags.
 * @param user_data User data.
 */
typedef void (*comparator_callback_t)(const struct device *dev,
				      uint32_t state,
				      void *user_data);

/** @brief Comparator configuration structure. */
struct comparator_cfg {
	/**
	 * Positive input specification.
	 * This field is composed of vendor-specific values.
	 */
	uint16_t input_positive;
	/**
	 * Negative input specification.
	 * This field is composed of vendor-specific values.
	 */
	uint16_t input_negative;
	/**
	 * Flags with additional configuration options.
	 * This field can contain COMPARATOR_FLAG_* values and vendor-specific
	 * flags.
	 */
	uint32_t flags;
};

/** @brief Driver-specific API functions to support comparator control. */
__subsystem struct comparator_driver_api {
	int (*configure)(const struct device *dev,
			 const struct comparator_cfg *cfg);
	int (*set_callback)(const struct device *dev,
			    comparator_callback_t callback,
			    void *user_data);
	int (*start)(const struct device *dev);
	int (*stop)(const struct device *dev);
	int (*get_state)(const struct device *dev, uint32_t *state);
};

/**
 * @brief Configure the comparator device.
 *
 * @param dev Comparator device instance.
 * @param cfg Configuration to be applied.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 */
__syscall int comparator_configure(const struct device *dev,
				   const struct comparator_cfg *cfg);

static inline int z_impl_comparator_configure(const struct device *dev,
					      const struct comparator_cfg *cfg)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->configure(dev, cfg);
}

/**
 * @brief Set callback function.
 *
 * @param dev Comparator device instance.
 * @param callback Callback function to be called on comparator state changes,
 *                 or NULL to disable the callback.
 * @param user_data Additional user data to be passed to the callback function.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 */
static inline int comparator_set_callback(const struct device *dev,
					  comparator_callback_t callback,
					  void *user_data)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->set_callback(dev, callback, user_data);
}

/**
 * @brief Start the comparator device.
 *
 * @param dev Comparator device instance.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 */
__syscall int comparator_start(const struct device *dev);

static inline int z_impl_comparator_start(const struct device *dev)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->start(dev);
}

/**
 * @brief Stop the comparator device.
 *
 * @param dev Comparator device instance.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 */
__syscall int comparator_stop(const struct device *dev);

static inline int z_impl_comparator_stop(const struct device *dev)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	return api->stop(dev);
}

/**
 * @brief Get the current state of the comparator device.
 *
 * @param dev Comparator device instance.
 * @param state Location where the current state bitfield is to be written.
 *              The written bitfield can contain COMPARATOR_STATE_* and
 *              vendor-specific flags.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -errno Other negative errno code in case of failure.
 */
__syscall int comparator_get_state(const struct device *dev, uint32_t *state);

static inline int z_impl_comparator_get_state(const struct device *dev,
					      uint32_t *state)
{
	const struct comparator_driver_api *api =
		(const struct comparator_driver_api *)dev->api;

	if (api->get_state == NULL) {
		return -ENOSYS;
	}

	return api->get_state(dev, state);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <syscalls/comparator.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COMPARATOR_H_ */
