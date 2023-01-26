/*
 * Copyright (c) Copyright (c) 2022 BrainCo Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <soc.h>

static int gd32l23x_init(const struct device *dev)
{
	uint32_t key;

	ARG_UNUSED(dev);

	key = irq_lock();

	SystemInit();
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(gd32l23x_init, PRE_KERNEL_1, 0);
