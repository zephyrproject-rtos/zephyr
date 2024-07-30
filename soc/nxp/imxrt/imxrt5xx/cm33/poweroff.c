/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <fsl_power.h>

static const uint32_t exclude_from_pd[] = {0, 0, 0, 0};

void z_sys_poweroff(void)
{
	/* Disable ISP Pin pull-ups and input buffers to avoid current leakage */
	IOPCTL->PIO[1][15] = 0;
	IOPCTL->PIO[3][28] = 0;
	IOPCTL->PIO[3][29] = 0;

	POWER_EnterDeepPowerDown(exclude_from_pd);

	CODE_UNREACHABLE;
}
