/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>
#include <device.h>
#include <drivers/clock_control.h>

#define LOG_MODULE_NAME bt_ctlr_llsw_openisa_lll_clock
#include "common/log.h"
#include "hal/debug.h"

int lll_clock_wait(void)
{
	return 0;
}
