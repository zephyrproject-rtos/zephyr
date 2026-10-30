/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/util.h>
#include <soc.h>

/* Bit n of the MSR and SWMRESS registers is unit n, taken from node label rlin3<n> or ostm<n> */
#define RLIN3_BIT(n, _) COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rlin3##n)), (BIT(n)), (0))
#define OSTM_BIT(n, _)  COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ostm##n)), (BIT(n)), (0))

#define RLIN3_BITS (LISTIFY(32, RLIN3_BIT, (|), _))
#define OSTM_BITS  (LISTIFY(10, OSTM_BIT, (|), _))

static void rh850_module_release(mem_addr_t swmress, mem_addr_t msr, uint32_t bits)
{
	while ((sys_read32(swmress) & bits) != 0U) {
	}
	sys_write32(RH850_KCPROT_ENABLE, RH850_MSRKCPROT);
	sys_write32(sys_read32(msr) & ~bits, msr);
	while ((sys_read32(msr) & bits) != 0U) {
	}
	sys_write32(RH850_KCPROT_DISABLE, RH850_MSRKCPROT);
	while (sys_read32(RH850_MSRKCPROT) != 0U) {
	}
}

/* PLL and CPU clock keep their reset settings */
void soc_early_init_hook(void)
{
	if (RLIN3_BITS != 0U) {
		sys_write32(RH850_KCPROT_ENABLE, RH850_CLKKCPROT1);
		sys_write32(RH850_CKS_RLIN_HSB, RH850_CKS_RLINC);
		while ((sys_read32(RH850_CKS_RLINS) & RH850_CKS_RLIN_MASK) != RH850_CKS_RLIN_HSB) {
		}
		sys_write32(RH850_KCPROT_DISABLE, RH850_CLKKCPROT1);

		rh850_module_release(RH850_SWMRESS_RLIN3, RH850_MSR_RLIN3, RLIN3_BITS);
	}

	if (OSTM_BITS != 0U) {
		rh850_module_release(RH850_SWMRESS_OSTM, RH850_MSR_OSTM, OSTM_BITS);
	}
}
