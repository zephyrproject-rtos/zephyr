/*
 * Copyright (c) 2018 omSquare s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include "SEGGER_RTT.h"

/*
 * Common mutex for locking access to terminal buffer.
 * Note that SEGGER uses same lock macros for both SEGGER_RTT_Write and
 * SEGGER_RTT_Read functions. Because of this we are not able generally
 * separate up and down access using two mutexes until SEGGER library fix
 * this.
 *
 * If sharing access cause performance problems, consider using another
 * non terminal buffers.
 */

K_MUTEX_DEFINE(rtt_term_mutex);

static int rtt_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	SEGGER_RTT_Init();

	return 0;
}

#ifdef CONFIG_MULTITHREADING

void zephyr_rtt_mutex_lock(void)
{
	k_mutex_lock(&rtt_term_mutex, K_FOREVER);
}

void zephyr_rtt_mutex_unlock(void)
{
	k_mutex_unlock(&rtt_term_mutex);
}

#endif /* CONFIG_MULTITHREADING */

unsigned int zephyr_rtt_irq_lock(void)
{
	return irq_lock();
}

void zephyr_rtt_irq_unlock(unsigned int key)
{
	irq_unlock(key);
}



SYS_INIT(rtt_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
