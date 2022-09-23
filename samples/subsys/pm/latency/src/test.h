/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>

struct test_api {
	void (*open)(const struct device *dev);
	void (*close)(const struct device *dev);
};

static inline void test_open(const struct device *dev)
{
	const struct test_api *api =
		(const struct test_api *)dev->api;

	api->open(dev);
}

static inline void test_close(const struct device *dev)
{
	const struct test_api *api =
		(const struct test_api *)dev->api;

	api->close(dev);
}
