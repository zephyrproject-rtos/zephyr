/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for entropy driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ENTROPY_H_
#error "Should only be included by zephyr/drivers/entropy.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_ENTROPY_INTERNAL_ENTROPY_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ENTROPY_INTERNAL_ENTROPY_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ENTROPY_INTERNAL_ENTROPY_IMPL_H_ */
