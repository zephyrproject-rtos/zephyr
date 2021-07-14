/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <arch/cpu.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int mimx8mm_a53_init(const struct device *arg)
{

	/* TODO */

	return 0;
}

SYS_INIT(mimx8mm_a53_init, PRE_KERNEL_1, 0);
