/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include "sensor_mgmt.h"

#define DT_DRV_COMPAT zephyr_sensing

LOG_MODULE_REGISTER(sensing, CONFIG_SENSING_LOG_LEVEL);

static struct sensing_mgmt_context sensing_ctx = {0};

static int sensing_init(void)
{
	return -ENOTSUP;
}

int sensing_get_sensors(int *sensor_nums, const struct sensing_sensor_info **info)
{
	return 0;
}

SYS_INIT(sensing_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
