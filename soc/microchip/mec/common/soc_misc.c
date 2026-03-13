/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>

#include "soc_misc.h"

/* EC Subsystem */
#define XEC_ECS_FEAT1_OFS              0x68U
#define XEC_ECS_FEAT1_CORE_CLK_48M_POS 19

/* eSPI TAF base address is the same for MEC/XEC families */
#define XEC_ESPI_TAF_REG_BASE       0x40008000U
#define XEC_ESPI_TAF_FC_MISC_OFS    0x38U
#define XEC_ESPI_TAF_FC_MISC_EN_POS 12

int soc_taf_enabled(void)
{
	mm_reg_t tfcm_addr = XEC_ESPI_TAF_REG_BASE + XEC_ESPI_TAF_FC_MISC_OFS;

	return sys_test_bit(tfcm_addr, XEC_ESPI_TAF_FC_MISC_EN_POS);
}

#if defined(CONFIG_SOC_SERIES_MEC15XX)
uint32_t soc_core_clock_get(void)
{
	return MHZ(48);
}
#else
uint32_t soc_core_clock_get(void)
{
	uint32_t core_clock_hz = MHZ(96);
#ifdef CONFIG_SOC_SERIES_MEC172X
	mm_reg_t pcr_base = (mm_reg_t)DT_REG_ADDR_BY_IDX(DT_NODELABEL(pcr), 0);

	if (sys_test_bit(pcr_base + XEC_PCR_TURBO_CLK_OFS, XEC_PCR_TURBO_CLK_EN_POS) == 0) {
		core_clock_hz = MHZ(48);
	}
#else
	mm_reg_t ecs_base = (mm_reg_t)DT_REG_ADDR(DT_NODELABEL(ecs));

	if (sys_test_bit(ecs_base + XEC_ECS_FEAT1_OFS, XEC_ECS_FEAT1_CORE_CLK_48M_POS) != 0) {
		core_clock_hz = MHZ(48);
	}
#endif
	return core_clock_hz;
}
#endif /* CONFIG_SOC_SERIES_MEC15XX */
