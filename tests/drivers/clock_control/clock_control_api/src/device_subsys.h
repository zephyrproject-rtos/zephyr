/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>

struct device_subsys_data {
	clock_control_subsys_t subsys;
	uint32_t startup_us;
};

struct device_data {
	const struct device *dev;
	const struct device_subsys_data *subsys_data;
	uint32_t subsys_cnt;
};
