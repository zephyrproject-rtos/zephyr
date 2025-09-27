/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	unsigned int key = irq_lock(); 

	/* Configure the Vector Table location */
#ifdef CONFIG_CPU_CORTEX_M_HAS_VTOR
	extern char _vector_start[];
    SCB->VTOR = (uintptr_t)_vector_start;  /* Linker guarantees correct alignment */
    __DSB(); __ISB();                      /* Ensure new vector table is used immediately */
#endif

#ifdef CONFIG_XIP
	xip_clock_switch(1);
#endif

	irq_unlock(key);
}
