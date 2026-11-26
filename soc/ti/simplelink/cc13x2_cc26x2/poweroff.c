/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <ti/drivers/Power.h>
#include <driverlib/gpio.h>

void z_sys_poweroff(void)
{
	uint32_t status;

	/* Power_shutdown() aborts if a wakeup-enabled pin has a pending edge event */
	status = HWREG(GPIO_BASE + GPIO_O_EVFLAGS31_0);
	HWREG(GPIO_BASE + GPIO_O_EVFLAGS31_0) = status;

	Power_shutdown(0, 0);

	CODE_UNREACHABLE;
}
