/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/__assert.h>
#include <kernel/zephyr/dpl/dpl.h>
#include <ti/drivers/dpl/ClockP.h>

uint32_t ClockP_getSystemTicks()
{
	return (uint32_t)_ms_to_ticks(k_uptime_get_32());
}

void ClockP_usleep(uint32_t usec)
{
	k_sleep((s32_t)usec);
}
