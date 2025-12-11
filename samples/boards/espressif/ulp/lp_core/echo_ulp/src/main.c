/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/poweroff.h>
#include <esp_sleep.h>
#include <ulp_lp_core.h>
#include <zephyr/kernel.h>

int main(void)
{
	while (1) {
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
