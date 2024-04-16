/*
 * Copyright (c) 2020, Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <cmsis_core.h>

void z_arm_tcm_disable_ecc(void)
{
#if defined(CONFIG_ARMV7_R)
	uint32_t actlr;

	actlr = __get_ACTLR();
	actlr &= ~(ACTLR_ATCMPCEN_Msk | ACTLR_B0TCMPCEN_Msk |
		ACTLR_B1TCMPCEN_Msk);
	__set_ACTLR(actlr);
#endif
}
