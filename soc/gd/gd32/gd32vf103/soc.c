/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <gd32vf103.h>

void soc_early_init_hook(void)
{
	uint32_t key;


	key = irq_lock();

	SystemInit();

	irq_unlock(key);
}
