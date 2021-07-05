/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BBRAM_H
#define ZEPHYR_INCLUDE_DRIVERS_BBRAM_H

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * API template to check if the BBRAM is invalid.
 *
 * @see z_impl_bbram_check_invlaid
 */
typedef int (*bbram_api_check_invalid)(const struct device *dev);

/**
 * API template to check for standby power failure.
 *
 * @see z_impl_bbram_check_standby_power
 */
typedef int (*bbram_api_check_standby_power)(const struct device *dev);

/**
 * API template to check for V CC1 power failure.
 *
 * @see z_impl_bbram_check_power
 */
typedef int (*bbram_api_check_power)(const struct device *dev);

/**
 * API template to read from BBRAM.
 *
 * @see z_impl_bbram_read
 */
typedef int (*bbram_api_read)(const struct device *dev, int offset, int size,
			      uint8_t *data);

/**
 * API template to write to BBRAM.
 *
 * @see z_impl_bbram_write
 */
typedef int (*bbram_api_write)(const struct device *dev, int offset, int size,
			       uint8_t *data);

__subsystem struct bbram_driver_api {
	bbram_api_check_invalid check_invalid;
	bbram_api_check_standby_power check_standby_power;
	bbram_api_check_power check_power;
	bbram_api_read read;
	bbram_api_write write;
};

__syscall int bbram_check_invalid(const struct device *dev);

/**
 * @brief Check if BBRAM is invalid
 *
 * Check if "Invalid Battery-Backed RAM" status is set then reset the status
 * bit. This may occur as a result to low voltage at the VBAT pin.
 *
 * @param dev BBRAM device pointer.
 * @return 0 if the Battery-Backed RAM data is valid, -EFAULT otherwise.
 */
static inline int z_impl_bbram_check_invalid(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_invalid) {
		return -ENOTSUP;
	}

	return api->check_invalid(dev);
}

__syscall int bbram_check_standby_power(const struct device *dev);

/**
 * @brief Check for standby (Volt SBY) power failure.
 *
 * Check if the V standby power domain is turned on after it was off then reset
 * the status bit.
 *
 * @param dev BBRAM device pointer.
 * @return 0 if V SBY power domain is in normal operation.
 */
static inline int z_impl_bbram_check_standby_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_standby_power) {
		return -ENOTSUP;
	}

	return api->check_standby_power(dev);
}

__syscall int bbram_check_power(const struct device *dev);

/**
 * @brief Check for V CC1 power failure.
 *
 * This will return an error if the V CC1 power domain is turned on after it was
 * off and reset the status bit.
 *
 * @param dev BBRAM device pointer.
 * @return 0 if the V CC1 power domain is in normal operation, -EFAULT
 * otherwise.
 */
static inline int z_impl_bbram_check_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_power) {
		return -ENOTSUP;
	}

	return api->check_power(dev);
}

__syscall int bbram_read(const struct device *dev, int offset, int size,
			 uint8_t *data);

/**
 * Read bytes from BBRAM.
 *
 * @param dev The BBRAM device pointer to read from.
 * @param offset The offset into the RAM address to start reading from.
 * @param size The number of bytes to read.
 * @param data The buffer to load the data into.
 * @return 0 on success, -EFAULT if the address range is out of bounds.
 */
static inline int z_impl_bbram_read(const struct device *dev, int offset,
				    int size, uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->read) {
		return -ENOTSUP;
	}

	return api->read(dev, offset, size, data);
}

__syscall int bbram_write(const struct device *dev, int offset, int size,
			  uint8_t *data);

/**
 * Write bytes to BBRAM.
 *
 * @param dev The BBRAM device pointer to write to.
 * @param offset The offset into the RAM address to start writing to.
 * @param size The number of bytes to write.
 * @param data Pointer to the start of data to write.
 * @return 0 on success, -EFAULT if the address range is out of bounds.
 */
static inline int z_impl_bbram_write(const struct device *dev, int offset,
				     int size, uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->write) {
		return -ENOTSUP;
	}

	return api->write(dev, offset, size, data);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/bbram.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BBRAM_H */
