/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup syscon_interface
 * @brief Main header file for SYSCON (System Control) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_

/**
 * @brief Interfaces for system control registers.
 * @defgroup syscon_interface System control (SYSCON)
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * API template to get the base address of the syscon region.
 *
 * @see syscon_get_base
 */
typedef int (*syscon_api_get_base)(const struct device *dev, uintptr_t *addr);

/**
 * API template to read a single register.
 *
 * @see syscon_read_reg
 */
typedef int (*syscon_api_read_reg)(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * API template to write a single register.
 *
 * @see syscon_write_reg
 */
typedef int (*syscon_api_write_reg)(const struct device *dev, uint16_t reg, uint32_t val);

/**
 * API template to get the size of the syscon register.
 *
 * @see syscon_get_size
 */
typedef int (*syscon_api_get_size)(const struct device *dev, size_t *size);

/**
 * @brief System Control (syscon) register driver API
 */
__subsystem struct syscon_driver_api {
	syscon_api_read_reg read;
	syscon_api_write_reg write;
	syscon_api_get_base get_base;
	syscon_api_get_size get_size;
};

/**
 * @brief Get the syscon base address
 *
 * @param dev The device to get the register size for.
 * @param addr Where to write the base address.
 * @return 0 When @a addr was written to.
 * @return -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_get_base(const struct device *dev, uintptr_t *addr);

static inline int z_impl_syscon_get_base(const struct device *dev, uintptr_t *addr)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if ((api == NULL) || (api->get_base == NULL)) {
		return -ENOSYS;
	}

	return api->get_base(dev, addr);
}


/**
 * @brief Read from syscon register
 *
 * This function reads from a specific register in the syscon area
 *
 * @param dev The device to get the register size for.
 * @param reg The register offset
 * @param val The returned value read from the syscon register
 *
 * @return 0 on success.
 * @return -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val);

static inline int z_impl_syscon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if ((api == NULL) || (api->read == NULL)) {
		return -ENOSYS;
	}

	return api->read(dev, reg, val);
}


/**
 * @brief Write to syscon register
 *
 * This function writes to a specific register in the syscon area
 *
 * @param dev The device to get the register size for.
 * @param reg The register offset
 * @param val The value to be written in the register
 *
 * @return 0 on success.
 * @return -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_write_reg(const struct device *dev, uint16_t reg, uint32_t val);

static inline int z_impl_syscon_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if ((api == NULL) || (api->write == NULL)) {
		return -ENOSYS;
	}

	return api->write(dev, reg, val);
}

/**
 * Get the size of the syscon register in bytes.
 *
 * @param dev The device to get the register size for.
 * @param size Pointer to write the size to.
 * @return 0 for success.
 * @return -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_get_size(const struct device *dev, size_t *size);

static inline int z_impl_syscon_get_size(const struct device *dev, size_t *size)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if ((api == NULL) || (api->get_size == NULL)) {
		return -ENOSYS;
	}

	return api->get_size(dev, size);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/syscon.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_ */
