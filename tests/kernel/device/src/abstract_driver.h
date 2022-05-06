/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/device.h>

/* define subsystem common API for drivers */
typedef int (*subsystem_do_this_t)(const struct device *device, int foo,
				   int bar);
typedef void (*subsystem_do_that_t)(const struct device *device,
				    unsigned int *baz);

struct subsystem_api {
	subsystem_do_this_t do_this;
	subsystem_do_that_t do_that;
};

static inline int subsystem_do_this(const struct device *device, int foo,
				    int bar)
{
	struct subsystem_api *api;

	api = (struct subsystem_api *)device->api;
	return api->do_this(device, foo, bar);
}

static inline void subsystem_do_that(const struct device *device,
				     unsigned int *baz)
{
	struct subsystem_api *api;

	api = (struct subsystem_api *)device->api;
	api->do_that(device, baz);
}
