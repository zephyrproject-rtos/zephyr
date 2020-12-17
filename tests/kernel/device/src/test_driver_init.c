/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <ztest.h>


/**
 * @brief Test cases to device driver initialization
 *
 */

#define MY_DRIVER_LV_1    "my_driver_level_1"
#define MY_DRIVER_LV_2    "my_driver_level_2"
#define MY_DRIVER_LV_3    "my_driver_level_3"
#define MY_DRIVER_LV_4    "my_driver_level_4"
#define MY_DRIVER_PRI_1    "my_driver_priority_1"
#define MY_DRIVER_PRI_2    "my_driver_priority_2"
#define MY_DRIVER_PRI_3    "my_driver_priority_3"
#define MY_DRIVER_PRI_4    "my_driver_priority_4"

#define LEVEL_PRE_KERNEL_1	1
#define LEVEL_PRE_KERNEL_2	2
#define LEVEL_POST_KERNEL	3
#define LEVEL_APPLICATION	4

#define PRIORITY_1	1
#define PRIORITY_2	2
#define PRIORITY_3	3
#define PRIORITY_4	4


/* this is for storing sequence during initializtion */
int init_level_sequence[4] = {0};
int init_priority_sequence[4] = {0};
unsigned int seq_level_cnt;
unsigned int seq_priority_cnt;

/* define driver type 1: for testing initialize levels and priorites */
typedef int (*my_api_configure_t)(const struct device *dev, int dev_config);

struct my_driver_api {
	my_api_configure_t configure;
};

static int my_configure(const struct device *dev, int config)
{
	return 0;
}

static const struct my_driver_api funcs_my_drivers = {
	.configure = my_configure,
};

/* driver init function of testing level */
static int my_driver_lv_1_init(const struct device *dev)
{
	init_level_sequence[seq_level_cnt] = LEVEL_PRE_KERNEL_1;
	seq_level_cnt++;

	return 0;
}

static int my_driver_lv_2_init(const struct device *dev)
{
	init_level_sequence[seq_level_cnt] = LEVEL_PRE_KERNEL_2;
	seq_level_cnt++;

	return 0;
}

static int my_driver_lv_3_init(const struct device *dev)
{
	init_level_sequence[seq_level_cnt] = LEVEL_POST_KERNEL;
	seq_level_cnt++;

	return 0;
}

static int my_driver_lv_4_init(const struct device *dev)
{
	init_level_sequence[seq_level_cnt] = LEVEL_APPLICATION;
	seq_level_cnt++;

	return 0;
}

/* driver init function of testing priority */
static int my_driver_pri_1_init(const struct device *dev)
{
	init_priority_sequence[seq_priority_cnt] = PRIORITY_1;
	seq_priority_cnt++;

	return 0;
}

static int my_driver_pri_2_init(const struct device *dev)
{
	init_priority_sequence[seq_priority_cnt] = PRIORITY_2;
	seq_priority_cnt++;

	return 0;
}

static int my_driver_pri_3_init(const struct device *dev)
{
	init_priority_sequence[seq_priority_cnt] = PRIORITY_3;
	seq_priority_cnt++;

	return 0;
}

static int my_driver_pri_4_init(const struct device *dev)
{
	init_priority_sequence[seq_priority_cnt] = PRIORITY_4;
	seq_priority_cnt++;

	return 0;
}

/**
 * @brief Test providing control device driver initialization order
 *
 * @details Test that kernel shall provide control over device driver
 * initalization order, using initialization level and priority for each
 * instance. We use DEVICE_DEFINE to define device instances and set
 * it's level and priority here, then we run check function later after
 * all of this instance finish their initialization.
 *
 * @ingroup kernel_device_tests
 */
DEVICE_DEFINE(my_driver_level_1, MY_DRIVER_LV_1, &my_driver_lv_1_init,
		device_pm_control_nop, NULL, NULL, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_level_2, MY_DRIVER_LV_2, &my_driver_lv_2_init,
		device_pm_control_nop, NULL, NULL, PRE_KERNEL_2,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_level_3, MY_DRIVER_LV_3, &my_driver_lv_3_init,
		device_pm_control_nop, NULL, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_level_4, MY_DRIVER_LV_4, &my_driver_lv_4_init,
		device_pm_control_nop, NULL, NULL, APPLICATION,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_1, MY_DRIVER_PRI_1,
		&my_driver_pri_1_init, device_pm_control_nop,
		NULL, NULL, POST_KERNEL, 1, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_2, MY_DRIVER_PRI_2,
		&my_driver_pri_2_init, device_pm_control_nop,
		NULL, NULL, POST_KERNEL, 2, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_3, MY_DRIVER_PRI_3,
		&my_driver_pri_3_init, device_pm_control_nop,
		NULL, NULL, POST_KERNEL, 3, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_4, MY_DRIVER_PRI_4,
		&my_driver_pri_4_init, device_pm_control_nop,
		NULL, NULL, POST_KERNEL, 4, &funcs_my_drivers);
