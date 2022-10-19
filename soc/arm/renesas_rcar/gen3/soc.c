/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

/**
 *
 * @brief Perform basic hardware initialization
 *
 * @return 0
 */

static int soc_init(void)
{

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();
	return 0;
}

void z_arm_platform_init(void)
{
	L1C_DisableCaches();
	L1C_DisableBTAC();

	/* Invalidate instruction cache and flush branch target cache */
	__set_ICIALLU(0);
	__DSB();
	__ISB();

	L1C_EnableCaches();
	L1C_EnableBTAC();
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
