/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

/* SYSCTL_BASE (0x400A_F000) + PWREN_MCPERIPH offset (0x1424) */
#define SYSCTL_PWREN_MCPERIPH (*(volatile uint32_t *)0x400B0424)

#define PWREN_KEY       (0x26 << 24)
#define PWREN_ECAP0_BIT BIT(3)
#define PWREN_ECAP1_BIT BIT(4)
#define PWREN_XBAR_BIT  BIT(14)

static int am13e_power_init(void)
{
	SYSCTL_PWREN_MCPERIPH = PWREN_KEY | SYSCTL_PWREN_MCPERIPH | PWREN_ECAP0_BIT |
				PWREN_ECAP1_BIT | PWREN_XBAR_BIT;

	return 0;
}
SYS_INIT(am13e_power_init, PRE_KERNEL_1, 0);
