/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>

#include <esp_sleep.h>
#include <hal/pmu_ll.h>

void z_sys_poweroff(void)
{
	/* Forces RTC domain to be always on */
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

#if CONFIG_ESP32_ULP_COPROC_ENABLED
	esp_sleep_enable_ulp_wakeup();
	/* Clear pending LP core trigger from initial boot kick */
	pmu_ll_hp_clear_sw_intr_status(&PMU);
#endif

	esp_deep_sleep_start();
}
