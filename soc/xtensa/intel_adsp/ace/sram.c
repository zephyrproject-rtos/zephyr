/* Copyright(c) 2021 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <soc_util.h>
#include <zephyr/cache.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <cpu_init.h>
#include "manifest.h"

#define DELAY_COUNT			256

__imr void hp_sram_init(uint32_t memory_size)
{
	ARG_UNUSED(memory_size);

	uint32_t hpsram_ebb_quantity = ace_hpsram_get_bank_count();
	uint32_t idx;

	for (idx = 0; idx < hpsram_ebb_quantity; ++idx) {
		HPSRAM_REGS(idx)->HSxPGCTL = 0;
		HPSRAM_REGS(idx)->HSxRMCTL = 1;
	}
	for (idx = 0; idx < hpsram_ebb_quantity; ++idx) {
		while (HPSRAM_REGS(idx)->HSxPGISTS != 0) {
		}
	}

	bbzero((void *)L2_SRAM_BASE, L2_SRAM_SIZE);
}

__imr void lp_sram_init(void)
{
	uint32_t lpsram_ebb_quantity = ace_lpsram_get_bank_count();
	uint32_t idx;

	for (idx = 0; idx < lpsram_ebb_quantity; ++idx) {
		LPSRAM_REGS(idx)->USxPGCTL = 0;
		LPSRAM_REGS(idx)->USxRMCTL = 1;
	}
	for (idx = 0; idx < lpsram_ebb_quantity; ++idx) {
		while (LPSRAM_REGS(idx)->USxPGISTS != 0) {
		}
	}

	bbzero((void *)LP_SRAM_BASE, LP_SRAM_SIZE);
}
