/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OPAMP_H_

/**
 * @brief Opamp Interface
 * @defgroup opamp_interface Opamp Interface
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/dt-bindings/opamp/opamp.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief OPAMP gain factors. */
enum opamp_gain {
	OPAMP_GAIN_1_7 = 0,	/**< x 1/7. */
	OPAMP_GAIN_1_3,		/**< x 1/3. */
	OPAMP_GAIN_1,		/**< x 1. */
	OPAMP_GAIN_5_3,		/**< x 5/3. */
	OPAMP_GAIN_2,		/**< x 2. */
	OPAMP_GAIN_11_5,	/**< x 11/5. */
	OPAMP_GAIN_3,		/**< x 3. */
	OPAMP_GAIN_4,		/**< x 4. */
	OPAMP_GAIN_13_3,	/**< x 13/3. */
	OPAMP_GAIN_7,		/**< x 7. */
	OPAMP_GAIN_8,		/**< x 8. */
	OPAMP_GAIN_15,		/**< x 15. */
	OPAMP_GAIN_16,		/**< x 16. */
	OPAMP_GAIN_31,		/**< x 31. */
	OPAMP_GAIN_32,		/**< x 32. */
	OPAMP_GAIN_33,		/**< x 33. */
	OPAMP_GAIN_63,		/**< x 63. */
	OPAMP_GAIN_64,		/**< x 64. */
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

typedef int (*opamp_api_set_gain_t)(const struct device *dev, enum opamp_gain gain);

__subsystem struct opamp_driver_api {
	opamp_api_set_gain_t set_gain;
};

/** @endcond */

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
