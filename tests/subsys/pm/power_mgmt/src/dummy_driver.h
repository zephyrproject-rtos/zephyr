/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#define DUMMY_NAME	"dummy_device"
#define DUMMY_PK_NAME	"dummy_pk_device"
#define DUMMY_WAKEUP_NAME DEVICE_DT_NAME(DT_INST(0, zephyr_wakeup_dev))
#define DUMMY_NO_PM	"dummy_no_pm_control_device"

typedef int (*dummy_api_t)(const struct device *dev);

struct dummy_driver_api {
	dummy_api_t open;
	dummy_api_t close;
	dummy_api_t refuse_to_sleep;
};
