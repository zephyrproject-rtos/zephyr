/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#define DUMMY_PARENT_NAME	"dummy_parent"

#define DUMMY_PARENT_RD		0
#define DUMMY_PARENT_WR		1

typedef int (*dummy_api_transfer_t)(const struct device *dev, uint32_t cmd,
				    uint32_t *val);

struct dummy_parent_api {
	dummy_api_transfer_t transfer;
};
