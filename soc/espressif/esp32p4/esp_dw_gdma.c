/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <hal/dw_gdma_ll.h>
#include <esp_dw_gdma.h>

#define ESP_DW_GDMA_CLK_TIMEOUT_US 1000

int esp_dw_gdma_init(void)
{
	static struct k_spinlock lock;
	static bool reset_done;
	dw_gdma_dev_t *dma = DW_GDMA_LL_GET_HW(0);
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!reset_done || !dw_gdma_ll_is_bus_clock_enabled(0)) {
		dw_gdma_ll_enable_bus_clock(0, true);
		if (!WAIT_FOR(dw_gdma_ll_is_bus_clock_enabled(0), ESP_DW_GDMA_CLK_TIMEOUT_US,
			      k_busy_wait(1))) {
			k_spin_unlock(&lock, key);
			return -ETIMEDOUT;
		}
		dw_gdma_ll_reset(dma);
		dw_gdma_ll_enable_controller(dma, true);
		dw_gdma_ll_enable_intr_global(dma, true);
		reset_done = true;
	}

	k_spin_unlock(&lock, key);

	return 0;
}
