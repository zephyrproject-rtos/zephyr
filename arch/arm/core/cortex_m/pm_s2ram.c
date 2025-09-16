/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 * Copyright (c) 2025 Nordic Semiconductor ASA
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

#if DT_NODE_EXISTS(DT_NODELABEL(pm_s2ram)) &&\
	DT_NODE_HAS_COMPAT(DT_NODELABEL(pm_s2ram), zephyr_memory_region)

/* Linker section name is given by `zephyr,memory-region` property of
 * `zephyr,memory-region` compatible DT node with nodelabel `pm_s2ram`.
 */
__attribute__((section(DT_PROP(DT_NODELABEL(pm_s2ram), zephyr_memory_region))))
#else
__noinit
#endif
_cpu_context_t _cpu_context;

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
