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

typedef int (*abstract_child_do_these_t)(const struct device *dev);

__subsystem struct abstract_child_driver_api {
	/* Embed parent API */
	struct abstract_driver_api abstract_api;

	abstract_child_do_these_t do_these;
};

/* Declare relation */
DEVICE_API_EXTENDS(abstract_child, abstract, abstract_api);

static inline int z_impl_abstract_child_do_these(const struct device *dev)
{
	return DEVICE_API_GET(abstract_child, dev)->do_these(dev);
}

__syscall int abstract_child_do_these(const struct device *dev);

/* Grandchild API: extends abstract_child (3-level inheritance) */
typedef int (*abstract_grandchild_do_all_t)(const struct device *dev, int val);

__subsystem struct abstract_grandchild_driver_api {
	/* Embed parent (child) API */
	struct abstract_child_driver_api abstract_child_api;

	abstract_grandchild_do_all_t do_all;
};

DEVICE_API_EXTENDS(abstract_grandchild, abstract_child, abstract_child_api);

static inline int z_impl_abstract_grandchild_do_all(const struct device *dev, int val)
{
	return DEVICE_API_GET(abstract_grandchild, dev)->do_all(dev, val);
}

__syscall int abstract_grandchild_do_all(const struct device *dev, int val);

/* Sibling API: also extends abstract (sibling of abstract_child) */
typedef int (*abstract_sibling_do_other_t)(const struct device *dev, int val);

__subsystem struct abstract_sibling_driver_api {
	/* Embed parent API */
	struct abstract_driver_api abstract_api;

	abstract_sibling_do_other_t do_other;
};

DEVICE_API_EXTENDS(abstract_sibling, abstract, abstract_api);

static inline int z_impl_abstract_sibling_do_other(const struct device *dev, int val)
{
	return DEVICE_API_GET(abstract_sibling, dev)->do_other(dev, val);
}

__syscall int abstract_sibling_do_other(const struct device *dev, int val);

#include <syscalls/abstract_driver.h>

#endif /* _ABSTRACT_DRIVER_H_ */
