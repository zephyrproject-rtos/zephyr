/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup otp_interface
 * @brief Main header file for OTP driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_OTP_H_
#define ZEPHYR_INCLUDE_DRIVERS_OTP_H_

/**
 * @brief Interfaces for One Time Programmable (OTP) Memory.
 * @defgroup otp_interface One Time Programmable Memory
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @brief Callback API upon reading from the OTP memory.
 * See @a otp_read() for argument description
 */
typedef int (*otp_api_read)(const struct device *dev, off_t offset, void *data, size_t len);

/**
 * @brief Callback API upon writing to the OTP memory.
 * See @a otp_program() for argument description
 */
typedef int (*otp_api_program)(const struct device *dev, off_t offset, const void *data,
			       size_t len);

__subsystem struct otp_driver_api {
	otp_api_read read;
#if defined(CONFIG_OTP_PROGRAM) || defined(__DOXYGEN__)
	otp_api_program program;
#endif
};

/** @endcond */

/**
 *  @brief Read data from OTP memory
 *
 *  @param dev OTP device
 *  @param offset Address offset to read from.
 *  @param data Buffer to store read data.
 *  @param len Number of bytes to read.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int otp_read(const struct device *dev, off_t offset, void *data, size_t len);

static inline int z_impl_otp_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	return DEVICE_API_GET(otp, dev)->read(dev, offset, data, len);
}

#if defined(CONFIG_OTP_PROGRAM) || defined(__DOXYGEN__)
/**
 *  @brief Program data to the given OTP memory
 *
 *  @param dev OTP device
 *  @param offset Address offset to program data to.
 *  @param data Buffer with data to program.
 *  @param len Number of bytes to program.
 *
 *  @return 0 on success, negative errno code on failure.
 */
__syscall int otp_program(const struct device *dev, off_t offset, const void *data, size_t len);

static inline int z_impl_otp_program(const struct device *dev, off_t offset, const void *data,
				      size_t len)
{
	const struct otp_driver_api *api = DEVICE_API_GET(otp, dev);

	if (api->program == NULL) {
		return -ENOSYS;
	}

	return api->program(dev, offset, data, len);
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/otp.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_OTP_H_ */
