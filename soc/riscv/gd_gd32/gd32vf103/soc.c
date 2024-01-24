/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <gd32vf103.h>

static int gigadevice_gd32v_soc_init(void)
{
	uint32_t key;


	key = irq_lock();

	SystemInit();

	irq_unlock(key);

	return 0;
}

SYS_INIT(gigadevice_gd32v_soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
