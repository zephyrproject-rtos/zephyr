/**
 * @file random.h
 *
 * @brief Public APIs for the random driver.
 */

/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RANDOM_H__
#define __RANDOM_H__

/**
 * @brief Random Interface
 * @defgroup random_interface Random Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <device.h>

/**
 * @typedef random_get_entropy_t
 * @brief Callback API to get entropy.
 *
 * See random_get_entropy() for argument description
 */
typedef int (*random_get_entropy_t)(struct device *dev,
				    uint8_t *buffer,
				    uint16_t length);

struct random_driver_api {
	random_get_entropy_t get_entropy;
};

/**
 * @brief Get entropy from the random driver.
 *
 * Fill a buffer with entropy from the random driver.
 *
 * @param dev Pointer to the random device.
 * @param buffer Buffer to fill with entropy.
 * @param length Buffer length.
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
static inline int random_get_entropy(struct device *dev,
				     uint8_t *buffer,
				     uint16_t length)
{
	const struct random_driver_api *api = dev->driver_api;

	__ASSERT(api->get_entropy, "Callback pointer should not be NULL");
	return api->get_entropy(dev, buffer, length);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __RANDOM_H__ */
