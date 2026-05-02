/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_subsys.h"
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

/* Test shared peripheral modules that use periph_module_enable/disable.
 *
 * Notes on module availability:
 * - UART1: shared only on ESP32, ESP32-S3, ESP32-C3
 * - UHCI0: has clock mask on all SoCs except ESP32-C2
 * - TIMG0: available on all SoCs
 *
 */
static const struct device_subsys_data subsys_data[] = {
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32S3) ||                      \
	defined(CONFIG_SOC_SERIES_ESP32C3)
	{.subsys = (void *)ESP32_UART1_MODULE},
#endif
#if !defined(CONFIG_SOC_SERIES_ESP32C2)
	{.subsys = (void *)ESP32_UHCI0_MODULE},
#endif
	{.subsys = (void *)ESP32_TIMG0_MODULE},
};

static const struct device_data devices[] = {
	{
		.dev = DEVICE_DT_GET_ONE(espressif_esp32_clock),
		.subsys_data =  subsys_data,
		.subsys_cnt = ARRAY_SIZE(subsys_data)
	}
};
