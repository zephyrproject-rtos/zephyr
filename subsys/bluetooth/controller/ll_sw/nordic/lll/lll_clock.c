/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>
#include <device.h>
#include <clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define LOG_MODULE_NAME bt_ctlr_llsw_nordic_lll_clock
#include "common/log.h"
#include "hal/debug.h"

#define DRV_NAME DT_NORDIC_NRF_CLOCK_0_LABEL "_32K"
#define K32SRC   CLOCK_CONTROL_NRF_K32SRC

static u8_t is_k32src_stable;

void lll_clock_wait(void)
{
	if (!is_k32src_stable) {
		struct device *clk_k32;

		is_k32src_stable = 1;

		clk_k32 = device_get_binding(DRV_NAME);
		LL_ASSERT(clk_k32);

		while (clock_control_on(clk_k32, (void *)K32SRC)) {
			DEBUG_CPU_SLEEP(1);
			k_cpu_idle();
			DEBUG_CPU_SLEEP(0);
		}
	}
}
