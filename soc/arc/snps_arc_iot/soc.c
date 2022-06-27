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

static int arc_iot_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (arc_iot_pll_fout_config(CPU_FREQ / 1000000) < 0) {
		return -1;
	}

	return 0;
}

SYS_INIT(arc_iot_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
