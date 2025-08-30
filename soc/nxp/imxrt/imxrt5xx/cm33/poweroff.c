/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <fsl_power.h>

static const uint32_t exclude_from_pd[] = {0, 0, 0, 0};

void z_sys_poweroff(void)
{
	CLOCK_EnableOsc32K(true);
	SYSCTL0->STARTEN1 |= SYSCTL0_STARTEN1_RTC_LITE0_WAKEUP_MASK;

	/* Disable ISP Pin pull-ups and input buffers to avoid current leakage */
	IOPCTL->PIO[1][15] = 0;
	IOPCTL->PIO[3][28] = 0;
	IOPCTL->PIO[3][29] = 0;

	POWER_EnterFullDeepPowerDown(exclude_from_pd);

	CODE_UNREACHABLE;
}
