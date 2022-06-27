/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int ls1046a_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/*todo: add soc init code here*/

	return 0;
}

SYS_INIT(ls1046a_init, PRE_KERNEL_1, 0);
