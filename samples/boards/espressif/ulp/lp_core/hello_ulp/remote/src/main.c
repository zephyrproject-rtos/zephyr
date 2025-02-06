/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include "ulp_lp_core_utils.h"

int main(void)
{
	printf("Hello World! %s\r\n", CONFIG_BOARD_TARGET);
	while(1) {
		k_msleep(2000);
		printf("Waking HP Core\n");
		ulp_lp_core_wakeup_main_processor();
	}

	return 0;
}
