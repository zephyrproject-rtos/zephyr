/*
 * Copyright (c) 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa_breadcrumb.h>
#include <adsp_memory.h>
#include <mem_window.h>
#include <zephyr/cache.h>

/* Select diagnostic data written to win0[1] (host "status/error code") */
#if defined(CONFIG_XTENSA_FATAL_BREADCRUMB_DATA_VADDR)
#define BC_DATA1(bsa, cause)	((uint32_t)(bsa)->excvaddr)
#elif defined(CONFIG_XTENSA_FATAL_BREADCRUMB_DATA_A0)
#define BC_DATA1(bsa, cause)	((uint32_t)(bsa)->a0)
#else
#define BC_DATA1(bsa, cause)	(0xe0000000U | ((uint32_t)(cause) & 0xffU))
#endif

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
		win0[1] = BC_DATA1(bsa, cause);
		win0[2] = (uint32_t)bsa->excvaddr;
		win0[4] = *(volatile uint32_t *)((uint32_t)bsa->pc & ~3U);
	}
	win0[3]++;
}
