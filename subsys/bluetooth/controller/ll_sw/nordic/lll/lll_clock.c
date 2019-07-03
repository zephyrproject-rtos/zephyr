/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>
#include <device.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define LOG_MODULE_NAME bt_ctlr_llsw_nordic_lll_clock
#include "common/log.h"
#include "hal/debug.h"

void lll_clock_wait(void)
{
	static bool done;

	if (done) {
		return;
	}
	done = true;

	struct device *lf_clock = device_get_binding(
		DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_32K");

	LL_ASSERT(lf_clock);

	clock_control_on(lf_clock, NULL);
	while (clock_control_get_status(lf_clock, NULL) !=
			CLOCK_CONTROL_STATUS_ON) {
		DEBUG_CPU_SLEEP(1);
		k_cpu_idle();
		DEBUG_CPU_SLEEP(0);
	}
}
