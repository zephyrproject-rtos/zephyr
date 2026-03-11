/*
 * Copyright (c) 2025 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_gpu2d

#include <zephyr/device.h>
#include <soc.h>
#include <fsl_clock.h>

static int gpu2d_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const clock_root_config_t gpu2d_clock_cfg = {
		.clockOff = false,
		.mux      = kCLOCK_GC355_ClockRoot_MuxVideoPllOut,
		.div      = 2,
	};

	CLOCK_SetRootClock(kCLOCK_Root_Gc355, &gpu2d_clock_cfg);
	CLOCK_GetRootClockFreq(kCLOCK_Root_Gc355);
	CLOCK_EnableClock(kCLOCK_Gpu2d);

	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(gpu2d), gpu2d_init, NULL,
		NULL, NULL, POST_KERNEL,
		CONFIG_APPLICATION_INIT_PRIORITY, NULL);
