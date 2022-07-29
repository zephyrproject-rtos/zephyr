/* Copyright(c) 2021 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/arch/xtensa/cache.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <cpu_init.h>
#include "manifest.h"
#include <ace_v1x-regs.h>


#define DELAY_COUNT			256

__imr void hp_sram_init(uint32_t memory_size)
{
	ARG_UNUSED(memory_size);

	uint32_t hpsram_ebb_quantity = mtl_hpsram_get_bank_count();
	volatile uint32_t *l2hsbpmptr = (volatile uint32_t *)MTL_L2MM->l2hsbpmptr;
	volatile uint8_t *status = (volatile uint8_t *)l2hsbpmptr + 4;
	int idx;

	for (idx = 0; idx < hpsram_ebb_quantity; ++idx) {
		*(l2hsbpmptr + idx * 2) = 0;
	}
	for (idx = 0; idx < hpsram_ebb_quantity; ++idx) {
		while (*(status + idx * 8) != 0) {
			z_idelay(DELAY_COUNT);
		}
	}
}

__imr void lp_sram_init(void)
{
	uint32_t lpsram_ebb_quantity = mtl_lpsram_get_bank_count();
	volatile uint32_t *l2usbpmptr = (volatile uint32_t *)MTL_L2MM->l2usbpmptr;

	for (uint32_t idx = 0; idx < lpsram_ebb_quantity; ++idx) {
		*(l2usbpmptr + idx * 2) = 0;
	}
}
