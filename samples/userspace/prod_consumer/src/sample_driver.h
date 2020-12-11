/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_FAKE_DRIVER_H
#define ZEPHYR_FAKE_DRIVER_H

#include <device.h>

#define SAMPLE_DRIVER_NAME_0	"SAMPLE_DRIVER_0"
#define SAMPLE_DRIVER_MSG_SIZE	128

typedef void (*sample_driver_callback_t)(const struct device *dev,
					 void *context, void *data);

typedef int (*sample_driver_write_t)(const struct device *dev, void *buf);

typedef int (*sample_driver_set_callback_t)(const struct device *dev,
					    sample_driver_callback_t cb,
					    void *context);

typedef int (*sample_driver_state_set_t)(const struct device *dev,
					 bool active);

__subsystem struct sample_driver_api {
	sample_driver_write_t write;
	sample_driver_set_callback_t set_callback;
	sample_driver_state_set_t state_set;
};

/*
 * Write some processed data to the sample driver
 *
 * Having done some processing on data received in the sample driver callback,
 * write this processed data back to the driver.
 *
 * @param dev Sample driver device
 * @param buf Processed data, of size SAMPLE_DRIVER_MSG_SIZE
 * @return 0 Success, nonzero if an error occurred
 */
__syscall int sample_driver_write(const struct device *dev, void *buf);

static inline int z_impl_sample_driver_write(const struct device *dev,
					     void *buf)
{
	const struct sample_driver_api *api = dev->api;

	return api->write(dev, buf);
}

/*
 * Set whether the sample driver will respond to interrupts
 *
 * @param dev Sample driver device
 * @param active Whether to activate/deactivate interrupts
 */
__syscall int sample_driver_state_set(const struct device *dev, bool active);

static inline int z_impl_sample_driver_state_set(const struct device *dev,
						 bool active)
{
	const struct sample_driver_api *api = dev->api;

	return api->state_set(dev, active);
}

/*
 * Register a callback function for the sample driver
 *
 * This callback runs in interrupt context. The provided data
 * blob will be of size SAMPLE_DRIVER_MSG_SIZE.
 *
 * @param dev Sample driver device to install callabck
 * @param cb Callback function pointer
 * @param context Context passed to callback function, or NULL if not needed
 * @return 0 Success, nonzero if an error occurred
 */
static inline int sample_driver_set_callback(const struct device *dev,
					     sample_driver_callback_t cb,
					     void *context)
{
	const struct sample_driver_api *api = dev->api;

	return api->set_callback(dev, cb, context);
}

#include <syscalls/sample_driver.h>

#endif
