/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for USB BC1.2 battery charging detect driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_USB_BC12_H_
#error "Should only be included by zephyr/drivers/usb/usb_bc12.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_USB_INTERNAL_USB_BC12_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_USB_INTERNAL_USB_BC12_IMPL_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_bc12_set_role(const struct device *dev, enum bc12_role role)
{
	const struct bc12_driver_api *api = (const struct bc12_driver_api *)dev->api;

	return api->set_role(dev, role);
}

static inline int z_impl_bc12_set_result_cb(const struct device *dev, bc12_callback_t cb,
					    void *user_data)
{
	const struct bc12_driver_api *api = (const struct bc12_driver_api *)dev->api;

	return api->set_result_cb(dev, cb, user_data);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USB_INTERNAL_USB_BC12_IMPL_H_ */
