/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>

#include "soc_misc.h"

/* eSPI TAF base address is the same for MEC/XEC families */
#define XEC_ESPI_TAF_REG_BASE       0x40008000U
#define XEC_ESPI_TAF_FC_MISC_OFS    0x38U
#define XEC_ESPI_TAF_FC_MISC_EN_POS 12

int soc_taf_enabled(void)
{
	mm_reg_t tfcm_addr = XEC_ESPI_TAF_REG_BASE + XEC_ESPI_TAF_FC_MISC_OFS;

	return sys_test_bit(tfcm_addr, XEC_ESPI_TAF_FC_MISC_EN_POS);
}
