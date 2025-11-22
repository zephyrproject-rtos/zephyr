/*
 * Copyright (c) 2025 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include "soc.h"

static void soc_crl_reset_ttc(void)
{
	sys_write32(0, CRL_RST_TTC);
}

void soc_early_init_hook(void)
{
	soc_crl_reset_ttc();
}

void relocate_vector_table(void)
{
	uint32_t sctlr = __get_SCTLR();

	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);
}
