/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *mux_control_t;

typedef int (*mux_configure_fn)(const struct device *dev,
				mux_control_t *control,
				void *data);

__subsystem struct mux_control_driver_api {
	mux_configure_fn	configure;
};

static inline int mux_control_configure(const struct device *dev,
					mux_control_t *control,
					void *data)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	return api->configure(dev, control, data);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_ */
