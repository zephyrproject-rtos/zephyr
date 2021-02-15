/*
 * Copyright (c) 2020, Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <arch/arm/aarch32/cortex_a_r/cmsis.h>

void z_arm_tcm_disable_ecc(void)
{
	uint32_t actlr;

	actlr = __get_ACTLR();
	actlr &= ~(ACTLR_ATCMPCEN_Msk | ACTLR_B0TCMPCEN_Msk |
		ACTLR_B1TCMPCEN_Msk);
	__set_ACTLR(actlr);
}
