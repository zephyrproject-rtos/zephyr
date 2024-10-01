/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This module provides routines to initialize and support soc-level hardware
 * for the IoT Development Kit board.
 *
 */
#include <zephyr/device.h>
#include <zephyr/init.h>
#include "sysconf.h"

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

void soc_early_init_hook(void)
{
	arc_iot_pll_fout_config(CPU_FREQ / 1000000);
}
