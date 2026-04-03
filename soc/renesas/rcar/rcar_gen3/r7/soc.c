/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/barrier.h>

void soc_reset_hook(void)
{
	L1C_DisableCaches();
	L1C_DisableBTAC();

	/* Invalidate instruction cache and flush branch target cache */
	__set_ICIALLU(0);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	L1C_EnableCaches();
	L1C_EnableBTAC();
}
