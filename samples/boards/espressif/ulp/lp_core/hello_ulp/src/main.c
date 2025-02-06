/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/poweroff.h>
#include <ulp_lp_core.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/hwinfo.h>

int main(void)
{
	uint32_t reset_cause;

	hwinfo_get_reset_cause(&reset_cause);

	switch (reset_cause) {
	case RESET_LOW_POWER_WAKE:
		printf("Wake up from deep sleep\n");
		break;
	default:
		printf("Not a deep sleep reset\n");
	}

	esp_sleep_enable_ulp_wakeup();
	printf("Powering off\n");
	sys_poweroff();

	return 0;
}
