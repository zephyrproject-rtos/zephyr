/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <nrf_sys_event.h>

static int nrf_const_lat(void)
{
	return nrf_sys_event_request_global_constlat();
}

/* Immediately after the SoC init functions */
SYS_INIT(nrf_const_lat, PRE_KERNEL_1, 1);
