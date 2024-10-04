/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include "SEGGER_RTT.h"

static int rtt_init(void)
{
	SEGGER_RTT_Init();

	return 0;
}

unsigned int zephyr_rtt_irq_lock(void)
{
	return irq_lock();
}

void zephyr_rtt_irq_unlock(unsigned int key)
{
	irq_unlock(key);
}

SYS_INIT(rtt_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
