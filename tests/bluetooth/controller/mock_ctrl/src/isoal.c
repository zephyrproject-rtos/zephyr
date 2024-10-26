/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

uint32_t isoal_get_wrapped_time_us(uint32_t time_now_us, int32_t time_diff_us)
{
	return time_now_us;
}
