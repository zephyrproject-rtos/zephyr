/*
 * Copyright 2022 Intel Corporation
 * Copyright 2023 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inline syscall implementations for I3C APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_H_
#error "Should only be included by zephyr/drivers/i3c.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_INTERNAL_I3C_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_INTERNAL_I3C_IMPL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#include <zephyr/drivers/i3c/addresses.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_i3c_do_ccc(const struct device *dev,
				    struct i3c_ccc_payload *payload)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->do_ccc == NULL) {
		return -ENOSYS;
	}

	return api->do_ccc(dev, payload);
}

static inline int z_impl_i3c_transfer(struct i3c_device_desc *target,
				      struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)target->bus->api;

	return api->i3c_xfers(target->bus, target, msgs, num_msgs);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_INTERNAL_I3C_IMPL_H_ */
