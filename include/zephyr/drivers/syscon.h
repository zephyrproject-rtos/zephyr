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
 * @def_driverbackendgroup{SYSCON, syscon_interface}
 * @{
 */

/**
 * @brief Get the base address of the syscon region.
 * See syscon_get_base() for argument description.
 */
typedef int (*syscon_api_get_base)(const struct device *dev, uintptr_t *addr);

/**
 * @brief Read a single register.
 * See syscon_read_reg() for argument description.
 */
typedef int (*syscon_api_read_reg)(const struct device *dev, uint16_t reg, uint32_t *val);

/**
 * @brief Write a single register.
 * See syscon_write_reg() for argument description.
 */
typedef int (*syscon_api_write_reg)(const struct device *dev, uint16_t reg, uint32_t val);

/**
 * @brief Atomically update bits in a register.
 * See syscon_update_bits() for argument description.
 */
typedef int (*syscon_api_update_bits)(const struct device *dev, uint16_t reg,
				      uint32_t mask, uint32_t val);

/**
 * @brief Get the size of the syscon register region.
 * See syscon_get_size() for argument description.
 */
typedef int (*syscon_api_get_size)(const struct device *dev, size_t *size);

/**
 * @driver_ops{SYSCON}
 */
__subsystem struct syscon_driver_api {
	/** @driver_ops_optional @copybrief syscon_read_reg */
	syscon_api_read_reg read;
	/** @driver_ops_optional @copybrief syscon_write_reg */
	syscon_api_write_reg write;
	/** @driver_ops_optional @copybrief syscon_update_bits */
	syscon_api_update_bits update_bits;
	/** @driver_ops_optional @copybrief syscon_get_base */
	syscon_api_get_base get_base;
	/** @driver_ops_optional @copybrief syscon_get_size */
	syscon_api_get_size get_size;
};

/** @} */

/**
 * @brief Get the syscon base address
 *
 * @param dev The device to get the register size for.
 * @param addr Where to write the base address.
 *
 * @retval 0 When @a addr was written to.
 * @retval -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_get_base(const struct device *dev, uintptr_t *addr);

static inline int z_impl_syscon_get_base(const struct device *dev, uintptr_t *addr)
{
	const struct syscon_driver_api *api = DEVICE_API_GET(syscon, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val);

static inline int z_impl_syscon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct syscon_driver_api *api = DEVICE_API_GET(syscon, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_write_reg(const struct device *dev, uint16_t reg, uint32_t val);

static inline int z_impl_syscon_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct syscon_driver_api *api = DEVICE_API_GET(syscon, dev);

	if ((api == NULL) || (api->write == NULL)) {
		return -ENOSYS;
	}

	return api->write(dev, reg, val);
}

/**
 * @brief Atomically update bits in a syscon register
 *
 * Performs a read-modify-write on a register under the driver's internal lock.
 * Bits selected by @a mask are cleared and replaced with the corresponding
 * bits from @a val.  Equivalent to:
 *
 *     syscon_read_reg(dev, reg, &tmp);
 *     tmp = (tmp & ~mask) | (val & mask);
 *     syscon_write_reg(dev, reg, tmp);
 *
 * but executed atomically with respect to other syscon operations on the
 * same device.
 *
 * @param dev The syscon device.
 * @param reg The register offset.
 * @param mask Bitmask of bits to modify.
 * @param val New values for the bits selected by @a mask.
 *
 * @retval 0 on success.
 * @retval -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_update_bits(const struct device *dev, uint16_t reg,
				 uint32_t mask, uint32_t val);

static inline int z_impl_syscon_update_bits(const struct device *dev, uint16_t reg,
					    uint32_t mask, uint32_t val)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if ((api == NULL) || (api->update_bits == NULL)) {
		return -ENOSYS;
	}

	return api->update_bits(dev, reg, mask, val);
}

/**
 * Get the size of the syscon register in bytes.
 *
 * @param dev The device to get the register size for.
 * @param size Pointer to write the size to.
 *
 * @retval 0 on success.
 * @retval -ENOSYS If the API or function isn't implemented.
 */
__syscall int syscon_get_size(const struct device *dev, size_t *size);

static inline int z_impl_syscon_get_size(const struct device *dev, size_t *size)
{
	const struct syscon_driver_api *api = DEVICE_API_GET(syscon, dev);

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
