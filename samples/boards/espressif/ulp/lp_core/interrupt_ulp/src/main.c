/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ulp_lp_core.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/poweroff.h>

int main(void)
{
	uint32_t cause;

	hwinfo_get_reset_cause(&cause);
	printf("Reset cause: 0x%x\n", cause);

	printf("Triggering ULP interrupt...\n");
	ulp_lp_core_sw_intr_trigger();
	k_sleep(K_MSEC(1000));

	printf("HP core entering deep sleep...\n");
	sys_poweroff();

	return 0;
}
