/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/poweroff.h>
#include <esp_attr.h>

static RTC_DATA_ATTR uint32_t wake_count;

int main(void)
{
	uint32_t cause;

	hwinfo_get_reset_cause(&cause);

	if (cause & RESET_LOW_POWER_WAKE) {
		wake_count++;
		printf("Woke up from LP core (LP timer)! Wake count: %d\n", wake_count);
	} else {
		wake_count = 0;
		printf("First boot, LP core will wake us every %d ms\n",
		       CONFIG_ESP32_ULP_LP_CORE_LP_TIMER_SLEEP_DURATION_US / 1000);
	}

	printf("HP core entering deep sleep...\n");
	sys_poweroff();

	return 0;
}
