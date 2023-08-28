/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BBRAM_H
#define ZEPHYR_INCLUDE_DRIVERS_BBRAM_H

#include <errno.h>

#include <zephyr/device.h>

/**
 * @brief BBRAM Interface
 * @defgroup bbram_interface BBRAM Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef bbram_api_check_invalid_t
 * @brief API template to check if the BBRAM is invalid.
 *
 * @see bbram_check_invalid
 */
typedef int (*bbram_api_check_invalid_t)(const struct device *dev);

/**
 * @typedef bbram_api_check_standby_power_t
 * @brief API template to check for standby power failure.
 *
 * @see bbram_check_standby_power
 */
typedef int (*bbram_api_check_standby_power_t)(const struct device *dev);

/**
 * @typedef bbram_api_check_power_t
 * @brief API template to check for V CC1 power failure.
 *
 * @see bbram_check_power
 */
typedef int (*bbram_api_check_power_t)(const struct device *dev);

/**
 * @typedef bbram_api_get_size_t
 * @brief API template to check the size of the BBRAM
 *
 * @see bbram_get_size
 */
typedef int (*bbram_api_get_size_t)(const struct device *dev, size_t *size);

/**
 * @typedef bbram_api_read_t
 * @brief API template to read from BBRAM.
 *
 * @see bbram_read
 */
typedef int (*bbram_api_read_t)(const struct device *dev, size_t offset, size_t size,
			      uint8_t *data);

/**
 * @typedef bbram_api_write_t
 * @brief API template to write to BBRAM.
 *
 * @see bbram_write
 */
typedef int (*bbram_api_write_t)(const struct device *dev, size_t offset, size_t size,
			       const uint8_t *data);

__subsystem struct bbram_driver_api {
	bbram_api_check_invalid_t check_invalid;
	bbram_api_check_standby_power_t check_standby_power;
	bbram_api_check_power_t check_power;
	bbram_api_get_size_t get_size;
	bbram_api_read_t read;
	bbram_api_write_t write;
};

/**
 * @brief Check if BBRAM is invalid
 *
 * Check if "Invalid Battery-Backed RAM" status is set then reset the status bit. This may occur as
 * a result to low voltage at the VBAT pin.
 *
 * @param[in] dev BBRAM device pointer.
 * @return 0 if the Battery-Backed RAM data is valid, -EFAULT otherwise.
 */
__syscall int bbram_check_invalid(const struct device *dev);

static inline int z_impl_bbram_check_invalid(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_invalid) {
		return -ENOTSUP;
	}

	return api->check_invalid(dev);
}

/**
 * @brief Check for standby (Volt SBY) power failure.
 *
 * Check if the V standby power domain is turned on after it was off then reset the status bit.
 *
 * @param[in] dev BBRAM device pointer.
 * @return 0 if V SBY power domain is in normal operation.
 */
__syscall int bbram_check_standby_power(const struct device *dev);

static inline int z_impl_bbram_check_standby_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_standby_power) {
		return -ENOTSUP;
	}

	return api->check_standby_power(dev);
}

/**
 * @brief Check for V CC1 power failure.
 *
 * This will return an error if the V CC1 power domain is turned on after it was off and reset the
 * status bit.
 *
 * @param[in] dev BBRAM device pointer.
 * @return 0 if the V CC1 power domain is in normal operation, -EFAULT otherwise.
 */
__syscall int bbram_check_power(const struct device *dev);

static inline int z_impl_bbram_check_power(const struct device *dev)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->check_power) {
		return -ENOTSUP;
	}

	return api->check_power(dev);
}

/**
 * @brief Get the size of the BBRAM (in bytes).
 *
 * @param[in] dev BBRAM device pointer.
 * @param[out] size Pointer to write the size to.
 * @return 0 for success, -EFAULT otherwise.
 */
__syscall int bbram_get_size(const struct device *dev, size_t *size);

static inline int z_impl_bbram_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->get_size) {
		return -ENOTSUP;
	}

	return api->get_size(dev, size);
}

/**
 * @brief Read bytes from BBRAM.
 *
 * @param[in]  dev The BBRAM device pointer to read from.
 * @param[in]  offset The offset into the RAM address to start reading from.
 * @param[in]  size The number of bytes to read.
 * @param[out] data The buffer to load the data into.
 * @return 0 on success, -EFAULT if the address range is out of bounds.
 */
__syscall int bbram_read(const struct device *dev, size_t offset, size_t size,
			 uint8_t *data);

static inline int z_impl_bbram_read(const struct device *dev, size_t offset,
				    size_t size, uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->read) {
		return -ENOTSUP;
	}

	return api->read(dev, offset, size, data);
}

/**
 * @brief Write bytes to BBRAM.
 *
 * @param[in]  dev The BBRAM device pointer to write to.
 * @param[in]  offset The offset into the RAM address to start writing to.
 * @param[in]  size The number of bytes to write.
 * @param[out] data Pointer to the start of data to write.
 * @return 0 on success, -EFAULT if the address range is out of bounds.
 */
__syscall int bbram_write(const struct device *dev, size_t offset, size_t size,
			  const uint8_t *data);

static inline int z_impl_bbram_write(const struct device *dev, size_t offset,
				     size_t size, const uint8_t *data)
{
	const struct bbram_driver_api *api =
		(const struct bbram_driver_api *)dev->api;

	if (!api->write) {
		return -ENOTSUP;
	}

	return api->write(dev, offset, size, data);
}

/**
 * @brief Set the emulated BBRAM driver's invalid state
 *
 * Calling this will affect the emulated behavior of bbram_check_invalid().
 *
 * @param[in] dev The emulated device to modify
 * @param[in] is_invalid The new invalid state
 * @return 0 on success, negative values on error.
 */
int bbram_emul_set_invalid(const struct device *dev, bool is_invalid);

/**
 * @brief Set the emulated BBRAM driver's standby power state
 *
 * Calling this will affect the emulated behavior of bbram_check_standby_power().
 *
 * @param[in] dev The emulated device to modify
 * @param[in] failure Whether or not standby power failure should be emulated
 * @return 0 on success, negative values on error.
 */
int bbram_emul_set_standby_power_state(const struct device *dev, bool failure);

/**
 * @brief Set the emulated BBRAM driver's  power state
 *
 * Calling this will affect the emulated behavior of bbram_check_power().
 *
 * @param[in] dev The emulated device to modify
 * @param[in] failure Whether or not a power failure should be emulated
 * @return 0 on success, negative values on error.
 */
int bbram_emul_set_power_state(const struct device *dev, bool failure);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/bbram.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BBRAM_H */
