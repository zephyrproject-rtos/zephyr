/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ABSTRACT_DRIVER_H_
#define _ABSTRACT_DRIVER_H_

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/sys/check.h>

/* define subsystem common API for drivers */
typedef int (*abstract_do_this_t)(const struct device *dev, int foo, int bar);
typedef void (*abstract_do_that_t)(const struct device *dev, unsigned int *baz);

__subsystem struct abstract_driver_api {
	abstract_do_this_t do_this;
	abstract_do_that_t do_that;
};

__syscall int abstract_do_this(const struct device *dev, int foo, int bar);

static inline int z_impl_abstract_do_this(const struct device *dev, int foo, int bar)
{
	__ASSERT_NO_MSG(DEVICE_API_IS(abstract, dev));

	return DEVICE_API_GET(abstract, dev)->do_this(dev, foo, bar);
}

__syscall void abstract_do_that(const struct device *dev, unsigned int *baz);

static inline void z_impl_abstract_do_that(const struct device *dev, unsigned int *baz)
{
	__ASSERT_NO_MSG(DEVICE_API_IS(abstract, dev));

	DEVICE_API_GET(abstract, dev)->do_that(dev, baz);
}

#include <syscalls/abstract_driver.h>

#endif /* _ABSTRACT_DRIVER_H_ */
