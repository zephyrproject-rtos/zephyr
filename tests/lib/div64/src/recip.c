/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Divisions by a constant that must use a reciprocal. CMakeLists.txt checks
 * that this object calls no 64-bit division routine, and that it does
 * without the optimizer.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/div64.h>

uint64_t div64_recip_div(uint64_t n);
uint64_t div64_recip_div_inline(uint64_t n);
uint32_t div64_recip_mod(uint64_t n);
int64_t div64_recip_sdiv(int64_t n);
uint64_t div64_recip_ns_to_ticks(uint64_t ns);

uint64_t div64_recip_div(uint64_t n)
{
	return sys_div64_u32_const(n, 1000) + sys_div64_u32_const(n, 7);
}

uint64_t div64_recip_div_inline(uint64_t n)
{
	return sys_div64_u32_const_inline(n, 7);
}

uint32_t div64_recip_mod(uint64_t n)
{
	return sys_mod64_u32_const(n, 1000000);
}

int64_t div64_recip_sdiv(int64_t n)
{
	return sys_sdiv64_u32_const(n, 60000);
}

uint64_t div64_recip_ns_to_ticks(uint64_t ns)
{
	return k_ns_to_ticks_floor64(ns);
}
