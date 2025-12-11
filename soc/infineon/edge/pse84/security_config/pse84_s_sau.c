/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse84_s_sau.h"

const cy_sau_config_t sau_config[CY_SAU_REGION_CNT] = {
	{
		.reg_num = 0U,
		.base_addr = 0x00000000U,
		.size = 0x10000000U,
		.end_addr = 0x0FFFFFFFU,
		.nsc = false,
	},
	{
		.reg_num = 1U,
		.base_addr = 0x20000000U,
		.size = 0x10000000U,
		.end_addr = 0x2FFFFFFFU,
		.nsc = false,
	},
	{
		.reg_num = 2U,
		.base_addr = 0x40000000U,
		.size = 0xc0000000U,
		.end_addr = 0xFFFFFFFFU,
		.nsc = false,
	},
};

void cy_sau_init(void)
{
	SAU->CTRL |= SAU_CTRL_ENABLE_Msk;
	for (uint8_t i = 0U; i < CY_SAU_REGION_CNT; i++) {
		SAU->RNR = sau_config[i].reg_num;
		SAU->RBAR = (sau_config[i].base_addr & SAU_RBAR_BADDR_Msk);
		SAU->RLAR = ((sau_config[i].end_addr & SAU_RLAR_LADDR_Msk) |
			     (sau_config[i].nsc ? SAU_RLAR_NSC_Msk : 0U) | SAU_RLAR_ENABLE_Msk);
	}
}
