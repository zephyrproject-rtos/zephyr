/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "core/time.h"

mc_time_t mc_now_u(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

mc_time_t mc_now_m(void)
{
	return (mc_time_t)k_uptime_get();
}

mc_time_t mc_now_s(void)
{
	return mc_now_m() / 1000U;
}

mc_time_t mc_now(void)
{
	return k_cyc_to_ns_floor64(k_cycle_get_64());
}

int usleep(unsigned int usec)
{
	k_usleep(usec);
	return 0;
}
