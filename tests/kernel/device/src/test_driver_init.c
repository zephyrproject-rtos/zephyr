/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/sections.h>


/**
 * @brief Test cases to device driver initialization
 *
 */

#define MY_DRIVER_LV_1    "my_driver_level_1"
#define MY_DRIVER_LV_2    "my_driver_level_2"
#define MY_DRIVER_LV_3    "my_driver_level_3"
#define MY_DRIVER_PRI_1    "my_driver_priority_1"
#define MY_DRIVER_PRI_2    "my_driver_priority_2"
#define MY_DRIVER_PRI_3    "my_driver_priority_3"
#define MY_DRIVER_PRI_4    "my_driver_priority_4"

#define LEVEL_PRE_KERNEL_1	1
#define LEVEL_PRE_KERNEL_2	2
#define LEVEL_POST_KERNEL	3

#define PRIORITY_1	1
#define PRIORITY_2	2
#define PRIORITY_3	3
#define PRIORITY_4	4


/* this is for storing sequence during initialization */
__pinned_bss int init_level_sequence[3] = {0};
__pinned_bss int init_priority_sequence[4] = {0};
__pinned_bss int init_sub_priority_sequence[3] = {0};
__pinned_bss unsigned int seq_level_cnt;
__pinned_bss unsigned int seq_priority_cnt;
__pinned_bss unsigned int seq_sub_priority_cnt;

/* define driver type 1: for testing initialize levels and priorities */
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
__pinned_func
static int my_driver_lv_1_init(const struct device *dev)
{
	init_level_sequence[seq_level_cnt] = LEVEL_PRE_KERNEL_1;
	seq_level_cnt++;

	return 0;
}

__pinned_func
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

/* driver init function of testing sub_priority */
static int my_driver_sub_pri_0_init(const struct device *dev)
{
	init_sub_priority_sequence[0] = seq_sub_priority_cnt++;

	return 0;
}

static int my_driver_sub_pri_1_init(const struct device *dev)
{
	init_sub_priority_sequence[1] = seq_sub_priority_cnt++;

	return 0;
}

static int my_driver_sub_pri_2_init(const struct device *dev)
{
	init_sub_priority_sequence[2] = seq_sub_priority_cnt++;

	return 0;
}

/**
 * @brief Test providing control device driver initialization order
 *
 * @details Test that kernel shall provide control over device driver
 * initialization order, using initialization level and priority for each
 * instance. We use DEVICE_DEFINE to define device instances and set
 * it's level and priority here, then we run check function later after
 * all of this instance finish their initialization.
 *
 * @ingroup kernel_device_tests
 */
DEVICE_DEFINE(my_driver_level_1, MY_DRIVER_LV_1, &my_driver_lv_1_init,
		NULL, NULL, NULL, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_level_2, MY_DRIVER_LV_2, &my_driver_lv_2_init,
		NULL, NULL, NULL, PRE_KERNEL_2,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

DEVICE_DEFINE(my_driver_level_3, MY_DRIVER_LV_3, &my_driver_lv_3_init,
		NULL, NULL, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs_my_drivers);

/* We use priority value of 20 to create a possible sorting conflict with
 * priority value of 2.  So if the linker sorting isn't working correctly
 * we'll find out.
 */
DEVICE_DEFINE(my_driver_priority_4, MY_DRIVER_PRI_4,
		&my_driver_pri_4_init, NULL, NULL, NULL, POST_KERNEL, 20,
		&funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_1, MY_DRIVER_PRI_1,
		&my_driver_pri_1_init, NULL, NULL, NULL, POST_KERNEL, 1,
		&funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_2, MY_DRIVER_PRI_2,
		&my_driver_pri_2_init, NULL, NULL, NULL, POST_KERNEL, 2,
		&funcs_my_drivers);

DEVICE_DEFINE(my_driver_priority_3, MY_DRIVER_PRI_3,
		&my_driver_pri_3_init, NULL, NULL, NULL, POST_KERNEL, 3,
		&funcs_my_drivers);

/* Create several devices at the same init priority that depend on each
 * other in devicetree so that we can validate linker sorting.
 */
DEVICE_DT_DEFINE(DT_NODELABEL(fakedomain_0), my_driver_sub_pri_0_init,
		NULL, NULL, NULL, POST_KERNEL, 33, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(fakedomain_1), my_driver_sub_pri_1_init,
		NULL, NULL, NULL, POST_KERNEL, 33, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(fakedomain_2), my_driver_sub_pri_2_init,
		NULL, NULL, NULL, POST_KERNEL, 33, NULL);
