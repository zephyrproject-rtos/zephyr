/*
 * Copyright (c) 2025 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include "soc.h"

void soc_reset_hook(void)
{
	uint32_t sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);
}
