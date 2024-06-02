/**
 * @file drivers/entropy.h
 *
 * @brief Public APIs for the entropy driver.
 */

/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_ENTROPY_H_
#define ZEPHYR_INCLUDE_DRIVERS_ENTROPY_H_

/**
 * @brief Entropy Interface
 * @defgroup entropy_interface Entropy Interface
 * @since 1.10
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Driver is allowed to busy-wait for random data to be ready */
#define ENTROPY_BUSYWAIT  BIT(0)

/**
 * @typedef entropy_get_entropy_t
 * @brief Callback API to get entropy.
 *
 * @note This call has to be thread safe to satisfy requirements
 * of the random subsystem.
 *
 * See entropy_get_entropy() for argument description
 */
typedef int (*entropy_get_entropy_t)(const struct device *dev,
				     uint8_t *buffer,
				     uint16_t length);
/**
 * @typedef entropy_get_entropy_isr_t
 * @brief Callback API to get entropy from an ISR.
 *
 * See entropy_get_entropy_isr() for argument description
 */
typedef int (*entropy_get_entropy_isr_t)(const struct device *dev,
					 uint8_t *buffer,
					 uint16_t length,
					 uint32_t flags);

/**
 * @brief Entropy driver API structure.
 *
 * This is the mandatory API any Entropy driver needs to expose.
 */
__subsystem struct entropy_driver_api {
	entropy_get_entropy_t     get_entropy;
	entropy_get_entropy_isr_t get_entropy_isr;
};

/**
 * @brief Fills a buffer with entropy. Blocks if required in order to
 *        generate the necessary random data.
 *
 * @param dev Pointer to the entropy device.
 * @param buffer Buffer to fill with entropy.
 * @param length Buffer length.
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
__syscall int entropy_get_entropy(const struct device *dev,
				  uint8_t *buffer,
				  uint16_t length);

static inline int z_impl_entropy_get_entropy(const struct device *dev,
					     uint8_t *buffer,
					     uint16_t length)
{
	const struct entropy_driver_api *api =
		(const struct entropy_driver_api *)dev->api;

	__ASSERT(api->get_entropy != NULL,
		"Callback pointer should not be NULL");
	return api->get_entropy(dev, buffer, length);
}

/**
 * @brief Fills a buffer with entropy in a non-blocking or busy-wait manner.
 * 	  Callable from ISRs.
 *
 * @param dev Pointer to the device structure.
 * @param buffer Buffer to fill with entropy.
 * @param length Buffer length.
 * @param flags Flags to modify the behavior of the call.
 * @retval number of bytes filled with entropy or -error.
 */
static inline int entropy_get_entropy_isr(const struct device *dev,
					  uint8_t *buffer,
					  uint16_t length,
					  uint32_t flags)
{
	const struct entropy_driver_api *api =
		(const struct entropy_driver_api *)dev->api;

	if (unlikely(!api->get_entropy_isr)) {
		return -ENOTSUP;
	}

	return api->get_entropy_isr(dev, buffer, length, flags);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/entropy.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ENTROPY_H_ */
