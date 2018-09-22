/**
 * @file entropy.h
 *
 * @brief Public APIs for the entropy driver.
 */

/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ENTROPY_H_
#define ZEPHYR_INCLUDE_ENTROPY_H_

/**
 * @brief Entropy Interface
 * @defgroup entropy_interface Entropy Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <device.h>

/**
 * @typedef entropy_get_entropy_t
 * @brief Callback API to get entropy.
 *
 * See entropy_get_entropy() for argument description
 */
typedef int (*entropy_get_entropy_t)(struct device *dev,
				    u8_t *buffer,
				    u16_t length);
/**
 * @typedef entropy_get_entropy_isr_t
 * @brief Callback API to get entropy from an ISR.
 *
 * See entropy_get_entropy_isr() for argument description
 */
typedef int (*entropy_get_entropy_isr_t)(struct device *dev,
					 u8_t *buffer,
					 u16_t length,
					 u32_t flags);
struct entropy_driver_api {
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
__syscall int entropy_get_entropy(struct device *dev,
				  u8_t *buffer,
				  u16_t length);

static inline int _impl_entropy_get_entropy(struct device *dev,
					    u8_t *buffer,
					    u16_t length)
{
	const struct entropy_driver_api *api = dev->driver_api;

	__ASSERT(api->get_entropy, "Callback pointer should not be NULL");
	return api->get_entropy(dev, buffer, length);
}

/* Busy-wait for random data to be ready */
#define ENTROPY_BUSYWAIT  BIT(0)

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
static inline int entropy_get_entropy_isr(struct device *dev,
					  u8_t *buffer,
					  u16_t length,
					  u32_t flags)
{
	const struct entropy_driver_api *api = dev->driver_api;

	if (!api->get_entropy_isr) {
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

#include <syscalls/entropy.h>

#endif /* ZEPHYR_INCLUDE_ENTROPY_H_ */
