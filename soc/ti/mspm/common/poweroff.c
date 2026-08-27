/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <ti/driverlib/driverlib.h>

void z_sys_poweroff(void)
{
	DL_SYSCTL_setPowerPolicySHUTDOWN();
	__WFI();

	CODE_UNREACHABLE;
}

static int ti_mspm0_poweroff_init(void)
{
	int ret;
	uint32_t rst_cause;

	ret = hwinfo_get_reset_cause(&rst_cause);
	if (ret != 0) {
		return ret;
	}

	if (RESET_LOW_POWER_WAKE == rst_cause) {
		DL_SYSCTL_releaseShutdownIO();
	}

	return 0;
}

SYS_INIT(ti_mspm0_poweroff_init, POST_KERNEL, 0);
