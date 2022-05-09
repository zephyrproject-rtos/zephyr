/*
 * Copyright (c) 2019-2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <soc.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_musca_s1_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise.
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_musca_s1_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
