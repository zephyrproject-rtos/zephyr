/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup opamp_interface
 * @brief Main header file for OPAMP (Operational Amplifier) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_

/**
 * @brief Interfaces for operational amplifiers (OPAMP).
 * @defgroup opamp_interface OPAMP
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief OPAMP gain factors. */
enum opamp_gain {
	OPAMP_GAIN_1_7 = 0, /**< x 1/7. */
	OPAMP_GAIN_1_3,     /**< x 1/3. */
	OPAMP_GAIN_1,       /**< x 1. */
	OPAMP_GAIN_5_3,     /**< x 5/3. */
	OPAMP_GAIN_2,       /**< x 2. */
	OPAMP_GAIN_11_5,    /**< x 11/5. */
	OPAMP_GAIN_3,       /**< x 3. */
	OPAMP_GAIN_4,       /**< x 4. */
	OPAMP_GAIN_13_3,    /**< x 13/3. */
	OPAMP_GAIN_7,       /**< x 7. */
	OPAMP_GAIN_8,       /**< x 8. */
	OPAMP_GAIN_15,      /**< x 15. */
	OPAMP_GAIN_16,      /**< x 16. */
	OPAMP_GAIN_31,      /**< x 31. */
	OPAMP_GAIN_32,      /**< x 32. */
	OPAMP_GAIN_33,      /**< x 33. */
	OPAMP_GAIN_63,      /**< x 63. */
	OPAMP_GAIN_64,      /**< x 64. */
};

/**
 * @def_driverbackendgroup{OPAMP,opamp_interface}
 * @{
 */

/**
 * @brief OPAMP functional mode.
 *
 * Values correspond 1:1 to the `functional-mode` property in
 * `dts/bindings/opamp/opamp-controller.yaml`.
 */
enum opamp_functional_mode {
	/** Differential amplifier mode */
	OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL = 0,
	/** Inverting amplifier mode */
	OPAMP_FUNCTIONAL_MODE_INVERTING,
	/** Non-inverting amplifier mode */
	OPAMP_FUNCTIONAL_MODE_NON_INVERTING,
	/** Follower mode */
	OPAMP_FUNCTIONAL_MODE_FOLLOWER,
	/**
	 * @brief Standalone mode.
	 * The gain is set by external resistors. The API call to set the gain
	 * is ignored in this mode or has no impact.
	 */
	OPAMP_FUNCTIONAL_MODE_STANDALONE,
};

/**
 * @brief Callback API to set opamp gain.
 * See opamp_set_gain() for argument description
 */
typedef int (*opamp_api_set_gain_t)(const struct device *dev, enum opamp_gain gain);

/**
 * @driver_ops{OPAMP}
 */
__subsystem struct opamp_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief opamp_set_gain
	 */
	opamp_api_set_gain_t set_gain;
};

/**
 * @}
 */

/**
 * @brief Set opamp gain.
 *
 * @param dev		Pointer to the device structure for the driver instance.
 * @param gain		Opamp gain, refer to enum @ref opamp_gain.
 *
 * @retval 0		If opamp gain has been successfully set.
 * @retval -errno	Negative errno in case of failure.
 */
__syscall int opamp_set_gain(const struct device *dev, enum opamp_gain gain);

static inline int z_impl_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	return DEVICE_API_GET(opamp, dev)->set_gain(dev, gain);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/opamp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_ */
