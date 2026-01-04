/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_ZEPHYR_INCLUDE_DEVICE_H_
#define MOCK_ZEPHYR_INCLUDE_DEVICE_H_

#include <stdbool.h>

#define DT_FOREACH_STATUS_OKAY_NODE(fn)
#define DT_ANY_INST_ON_BUS_STATUS_OKAY(x) 0
#define DT_INST_FOREACH_STATUS_OKAY(x)
#define DEVICE_API(_class, _name) struct sensor_driver_api adxl345_api_funcs
struct device {
	/** Name of the device instance */
	const char *name;
	/** Address of device instance config information */
	const void *config;
	/** Address of the API structure exposed by the device instance */
	const void *api;
	/** Address of the device instance private data */
	void *data;
};

static inline bool device_is_ready(const struct device *dev)
{
	return true;
}
#endif /* MOCK_ZEPHYR_INCLUDE_DEVICE_H_ */
