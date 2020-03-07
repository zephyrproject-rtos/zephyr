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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll_clock
#include "common/log.h"
#include "hal/debug.h"

void lll_clock_wait(void)
{
	static bool done;

	if (done) {
		return;
	}
	done = true;

	struct device *clock =
			device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL);

	LL_ASSERT(clock);

	clock_control_on(clock, CLOCK_CONTROL_NRF_SUBSYS_LF);
	while (clock_control_get_status(clock, CLOCK_CONTROL_NRF_SUBSYS_LF) !=
			CLOCK_CONTROL_STATUS_ON) {
		DEBUG_CPU_SLEEP(1);
		k_cpu_idle();
		DEBUG_CPU_SLEEP(0);
	}
}
