/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

#include <zephyr/arch/common/pm_s2ram.h>

/**
 * CPU context for S2RAM
 */
__noinit _cpu_context_t _cpu_context;

/**
 * S2RAM Marker
 */
__noinit uint32_t marker;
