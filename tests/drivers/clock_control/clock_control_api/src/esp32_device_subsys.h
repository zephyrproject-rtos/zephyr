/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

/* Test common modules among espressif SOCs*/
static const struct device_subsys_data subsys_data[] = {
	{.subsys = (void *) ESP32_LEDC_MODULE},
	{.subsys = (void *) ESP32_UART1_MODULE},
	{.subsys = (void *) ESP32_I2C0_MODULE},
	{.subsys = (void *) ESP32_UHCI0_MODULE},
	{.subsys = (void *) ESP32_RMT_MODULE},
	{.subsys = (void *) ESP32_TWAI_MODULE},
	{.subsys = (void *) ESP32_RNG_MODULE},
};

static const struct device_data devices[] = {
	{
		.dev = DEVICE_DT_GET_ONE(espressif_esp32_rtc),
		.subsys_data =  subsys_data,
		.subsys_cnt = ARRAY_SIZE(subsys_data)
	}
};
