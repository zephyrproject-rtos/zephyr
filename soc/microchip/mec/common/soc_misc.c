/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>

#include "soc_misc.h"

#if defined(CONFIG_SOC_SERIES_MEC15XX) || defined(CONFIG_SOC_SERIES_MEC172X)
#define XEC_ESPI_TAF_REG_BASE DT_REG_ADDR(DT_NODELABEL(espi_saf0))
#else
#define XEC_ESPI_TAF_REG_BASE DT_REG_ADDR(DT_NODELABEL(espi_taf0))
#endif
#define XEC_ESPI_TAF_FC_MISC_OFS    0x38u
#define XEC_ESPI_TAF_FC_MISC_EN_POS 12

int soc_taf_enabled(void)
{
	mm_reg_t tfcm_addr = XEC_ESPI_TAF_REG_BASE + XEC_ESPI_TAF_FC_MISC_OFS;

	return sys_test_bit(tfcm_addr, XEC_ESPI_TAF_FC_MISC_EN_POS);
}
