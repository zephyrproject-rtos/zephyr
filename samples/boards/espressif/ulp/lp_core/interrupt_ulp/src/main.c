/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <ulp_lp_core.h>
#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		printf("Triggering ULP interrupt...\n");
		ulp_lp_core_sw_intr_trigger();
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
