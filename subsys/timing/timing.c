/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <kernel.h>
#include <sys/atomic.h>
#include <timing/timing.h>

static bool has_inited;
static atomic_val_t started_ref;

void timing_init(void)
{
	if (has_inited) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_init();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_init();
#else
	arch_timing_init();
#endif

	has_inited = true;
}

void timing_start(void)
{
	if (atomic_inc(&started_ref) != 0) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_start();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_start();
#else
	arch_timing_start();
#endif
}

void timing_stop(void)
{
	if (atomic_dec(&started_ref) > 1) {
		return;
	}

#if defined(CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
	board_timing_stop();
#elif defined(CONFIG_SOC_HAS_TIMING_FUNCTIONS)
	soc_timing_stop();
#else
	arch_timing_stop();
#endif
}
