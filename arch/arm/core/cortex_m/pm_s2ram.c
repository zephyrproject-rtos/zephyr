/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

#include <zephyr/arch/common/pm_s2ram.h>

#define MAGIC (0xDABBAD00)

/**
 * CPU context for S2RAM
 */
__noinit _cpu_context_t _cpu_context;

#ifndef CONFIG_PM_S2RAM_CUSTOM_MARKING
/**
 * S2RAM Marker
 */
static __noinit uint32_t marker;

void pm_s2ram_mark_set(void)
{
	marker = MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	if (marker == MAGIC) {
		marker = 0;

		return true;
	}

	return false;
}

#endif /* CONFIG_PM_S2RAM_CUSTOM_MARKING */
