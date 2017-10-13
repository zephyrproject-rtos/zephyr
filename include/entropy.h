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
#ifndef __ENTROPY_H__
#define __ENTROPY_H__

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

struct entropy_driver_api {
	entropy_get_entropy_t get_entropy;
};

/**
 * @brief Fills a buffer with entropy.
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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/entropy.h>

#endif /* __ENTROPY_H__ */
