/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include "abstract_driver.h"

#define MY_DRIVER_A		"my_driver_A"
#define MY_DRIVER_B		"my_driver_B"
#define MY_CHILD_DRIVER		"my_child_driver"
#define MY_GRANDCHILD_DRIVER	"my_grandchild_driver"
#define MY_SIBLING_DRIVER	"my_sibling_driver"

/* define individual driver A */
static int my_driver_A_do_this(const struct device *dev, int foo, int bar)
{
	return foo + bar;
}

static void my_driver_A_do_that(const struct device *dev, unsigned int *baz)
{
	*baz = 1;
}

static DEVICE_API(abstract, my_driver_A_api_funcs) = {
	.do_this = my_driver_A_do_this,
	.do_that = my_driver_A_do_that,
};

int common_driver_init(const struct device *dev)
{
	return 0;
}

/* define individual driver B */
static int my_driver_B_do_this(const struct device *dev, int foo, int bar)
{
	return foo - bar;
}

static void my_driver_B_do_that(const struct device *dev, unsigned int *baz)
{
	*baz = 2;
}

static DEVICE_API(abstract, my_driver_B_api_funcs) = {
	.do_this = my_driver_B_do_this,
	.do_that = my_driver_B_do_that,
};

/* define child driver */
static int child_driver_do_this(const struct device *dev, int foo, int bar)
{
	return foo * bar;
}

static void child_driver_do_that(const struct device *dev, unsigned int *baz)
{
	*baz = 3;
}

static int child_driver_do_these(const struct device *dev)
{
	unsigned int baz;

	/* Make it full-circle and call parent APIs */
	abstract_do_that(dev, &baz);
	return abstract_do_this(dev, (int)baz, (int)baz);
}

static DEVICE_API(abstract_child, child_driver_api_funcs) = {
	.abstract_api = {
		.do_this = child_driver_do_this,
		.do_that = child_driver_do_that,
	},
	.do_these = child_driver_do_these,
};

/* define grandchild driver (extends child, which extends abstract) */
static int grandchild_driver_do_this(const struct device *dev, int foo, int bar)
{
	return foo + bar + 1;
}

static void grandchild_driver_do_that(const struct device *dev, unsigned int *baz)
{
	*baz = 4;
}

static int grandchild_driver_do_these(const struct device *dev)
{
	unsigned int baz;

	abstract_do_that(dev, &baz);
	return abstract_do_this(dev, (int)baz, (int)baz);
}

static int grandchild_driver_do_all(const struct device *dev, int val)
{
	/* Combine all levels: call parent APIs and add own contribution */
	int ret = abstract_child_do_these(dev);

	return ret + val;
}

static DEVICE_API(abstract_grandchild, grandchild_driver_api_funcs) = {
	.abstract_child_api = {
		.abstract_api = {
			.do_this = grandchild_driver_do_this,
			.do_that = grandchild_driver_do_that,
		},
		.do_these = grandchild_driver_do_these,
	},
	.do_all = grandchild_driver_do_all,
};

/* define sibling driver (extends abstract, sibling of child) */
static int sibling_driver_do_this(const struct device *dev, int foo, int bar)
{
	return foo % bar;
}

static void sibling_driver_do_that(const struct device *dev, unsigned int *baz)
{
	*baz = 5;
}

static int sibling_driver_do_other(const struct device *dev, int val)
{
	return val * 10;
}

static DEVICE_API(abstract_sibling, sibling_driver_api_funcs) = {
	.abstract_api = {
		.do_this = sibling_driver_do_this,
		.do_that = sibling_driver_do_that,
	},
	.do_other = sibling_driver_do_other,
};

/**
 * @cond INTERNAL_HIDDEN
 */
DEVICE_DEFINE(my_driver_A, MY_DRIVER_A, &common_driver_init,
		NULL, NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&my_driver_A_api_funcs);

DEVICE_DEFINE(my_driver_B, MY_DRIVER_B, &common_driver_init,
		NULL, NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&my_driver_B_api_funcs);

DEVICE_DEFINE(my_child_driver, MY_CHILD_DRIVER, &common_driver_init,
		NULL, NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&child_driver_api_funcs);

DEVICE_DEFINE(my_grandchild_driver, MY_GRANDCHILD_DRIVER, &common_driver_init,
		NULL, NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&grandchild_driver_api_funcs);

DEVICE_DEFINE(my_sibling_driver, MY_SIBLING_DRIVER, &common_driver_init,
		NULL, NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&sibling_driver_api_funcs);
/**
 * @endcond
 */
