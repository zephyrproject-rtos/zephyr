/*
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INFINEON_AURIX_TC3X_SOC_TOS_H_
#define ZEPHYR_SOC_INFINEON_AURIX_TC3X_SOC_TOS_H_

#ifndef _ASMLANGUAGE

#include <stdint.h>

enum aurix_tos {
	AURIX_TOS_CPU0 = 0x00U,
	AURIX_TOS_DMA = 0x01U,
	AURIX_TOS_CPU1 = 0x02U,
	AURIX_TOS_CPU2 = 0x03U,
	AURIX_TOS_CPU3 = 0x04U,
	AURIX_TOS_CPU4 = 0x05U,
	AURIX_TOS_CPU5 = 0x06U
};

static inline enum aurix_tos aurix_coreid_to_tos(uint32_t core_id)
{
	return core_id == 0 ? core_id : core_id + 1;
}

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_SOC_INFINEON_AURIX_TC3X_SOC_TOS_H_ */
