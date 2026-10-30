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

#define RLIN311_BIT BIT(11)
#define OSTM0_BIT   BIT(0)

static void u2c_module_release(mem_addr_t swmress, mem_addr_t msr, uint32_t bit)
{
	while ((sys_read32(swmress) & bit) != 0U) {
	}
	sys_write32(U2C_KCPROT_ENABLE, U2C_MSRKCPROT);
	sys_write32(sys_read32(msr) & ~bit, msr);
	while ((sys_read32(msr) & bit) != 0U) {
	}
	sys_write32(U2C_KCPROT_DISABLE, U2C_MSRKCPROT);
	while (sys_read32(U2C_MSRKCPROT) != 0U) {
	}
}

/* PLL and CPU clock keep their reset settings */
void soc_early_init_hook(void)
{
	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rlin311))) {
		sys_write32(U2C_KCPROT_ENABLE, U2C_CLKKCPROT1);
		sys_write32(U2C_CKS_RLIN_HSB, U2C_CKS_RLINC);
		while ((sys_read32(U2C_CKS_RLINS) & 0x7U) != U2C_CKS_RLIN_HSB) {
		}
		sys_write32(U2C_KCPROT_DISABLE, U2C_CLKKCPROT1);

		u2c_module_release(U2C_SWMRESS_RLIN3, U2C_MSR_RLIN3, RLIN311_BIT);
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ostm0))) {
		u2c_module_release(U2C_SWMRESS_OSTM, U2C_MSR_OSTM, OSTM0_BIT);
	}
}
