/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT5XX soc f1 core
 *
 */

#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/xtensa/xtensa_ptr.h>

/* replaces weak version in xtensa_backtrace.c */
bool xtensa_soc_ptr_executable(const void *p)
{
	return ((p >= (void *)__text_region_start) &&
		(p <= (void *)__text_region_end));
}
