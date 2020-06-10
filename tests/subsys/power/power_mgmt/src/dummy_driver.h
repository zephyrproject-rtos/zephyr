/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#define DUMMY_DRIVER_NAME	"dummy_driver"

typedef int (*dummy_api_open_t)(struct device *dev);

typedef int (*dummy_api_close_t)(struct device *dev);

struct dummy_driver_api {
	dummy_api_open_t open;
	dummy_api_close_t close;
};
