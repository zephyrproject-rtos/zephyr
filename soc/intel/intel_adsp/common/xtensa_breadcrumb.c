/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_breadcrumb.h>
#include <adsp_memory.h>
#include <mem_window.h>
#include <zephyr/cache.h>

/*
 * Record a fatal exception into HP-SRAM window0. Only the first fatal
 * exception is latched; win0[3] counts all.
 */
void xtensa_fatal_breadcrumb(const _xtensa_irq_bsa_t *bsa, int cause)
{
	volatile uint32_t *win0 =
		(volatile uint32_t *)sys_cache_uncached_ptr_get((void *)HP_SRAM_WIN0_BASE);

	if (win0[0] == 0U) {
		win0[0] = (uint32_t)bsa->pc;
		win0[1] = 0xe0000000U | ((uint32_t)cause & 0xffU);
		win0[2] = (uint32_t)bsa->excvaddr;
		win0[4] = *(volatile uint32_t *)((uint32_t)bsa->pc & ~3U);
	}
	win0[3]++;
}
