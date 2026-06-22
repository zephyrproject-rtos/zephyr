/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I3C_ENDPOINT_H_
#define ZEPHYR_MCTP_I3C_ENDPOINT_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>

/** @cond INTERNAL_HIDDEN */
struct mctp_binding_i3c_controller;

struct mctp_i3c_endpoint_api {
	void (*bind)(const struct device *dev, struct mctp_binding_i3c_controller *ctlr,
		    struct i3c_device_desc **i3c_dev);
	struct mctp_binding_i3c_controller *(*binding)(const struct device *dev);
};

static inline void mctp_i3c_endpoint_bind(const struct device *dev,
					  struct mctp_binding_i3c_controller *ctlr,
					  struct i3c_device_desc **i3c_dev)
{
	const struct mctp_i3c_endpoint_api *api = dev->api;

	api->bind(dev, ctlr, i3c_dev);
}

static inline struct mctp_binding_i3c_controller *mctp_i3c_endpoint_binding(const struct device *d)
{
	const struct mctp_i3c_endpoint_api *api = d->api;

	return api->binding(d);
}
/** @endcond INTERNAL_HIDDEN */

#endif /* ZEPHYR_MCTP_I3C_ENDPOINT_H_ */
