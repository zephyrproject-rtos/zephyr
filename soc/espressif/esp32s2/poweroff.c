/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>

#include <esp_sleep.h>

void z_sys_poweroff(void)
{
	/* Forces RTC domain to be always on */
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	esp_deep_sleep_start();
}
