/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Temperature sensor driver for STM32F401x chips.
 *
 * @warning Temperature readings should be use for RELATIVE
 *          TEMPERATURE CHANGES ONLY.
 *
 *          Inter-chip temperature sensor readings may vary by as much
 *          as 45 degrees C.
 */

#include <device.h>
#include <sensor.h>

/* TODO */
static int temp_stm32f401x_sample_fetch(struct device *dev,
					enum sensor_channel chan)
{
	return 0;
}

/* TODO */
static int temp_stm32f401x_channel_get(struct device *dev,
				       enum sensor_channel chan,
				       struct sensor_value *val)
{
	val->val1 = 0;
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api temp_stm32f401x_driver_api = {
	.sample_fetch = temp_stm32f401x_sample_fetch,
	.channel_get = temp_stm32f401x_channel_get,
};

/* TODO */
static int temp_stm32f401x_init(struct device *dev)
{
	return 0;
}

DEVICE_AND_API_INIT(temp_stm32f401x, CONFIG_TEMP_STM32F401X_NAME,
		    temp_stm32f401x_init, NULL, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &temp_stm32f401x_driver_api);
