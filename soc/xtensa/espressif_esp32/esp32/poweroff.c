/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>

#include <esp_sleep.h>
#include <hal/rtc_io_hal.h>
#include <soc/rtc_io_channel.h>

void z_sys_poweroff(void)
{
	/*
	 * Isolate GPIO12 pin from external circuits. This is needed for modules
	 * which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
	 * to minimize current consumption.
	 */
	rtcio_hal_isolate(RTCIO_GPIO12_CHANNEL);
	/* Forces RTC domain to be always on */
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	esp_deep_sleep_start();
}
