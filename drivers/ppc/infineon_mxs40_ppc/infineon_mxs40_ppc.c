/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/ppc/ppc.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include "cy_ppc.h"

LOG_MODULE_REGISTER(infineon_mxs40_ppc);

/*
 * Grant non-secure access to a specific list of PPC peripheral regions.
 *
 * This mirrors what TF-M / the Infineon BSP does (init_cycfg_ppc0/1 →
 * Cy_PPC_Init over the "M33_M55" non-secure domain): it configures only the
 * curated set of peripheral regions the non-secure world needs — notably the
 * console SCB — rather than sweeping every region.  A blanket sweep bus-faults
 * because it touches self-protection / SROM-locked slots and writes regions
 * that the Secure protection context is not permitted to modify.
 *
 * The Cy_Ppc_InitPpc() CTL write is deliberately NOT issued here: MXS40
 * silicon restricts PPC CTL writes to the SROM protection context, so we leave
 * the SROM-configured bus-error response in place.
 */
static void ppc_open_regions(PPC_Type *base, uint32_t addr, const uint32_t *regions, size_t count,
			     uint32_t pc_mask)
{
	cy_stc_ppc_attribute_t cfg = {
		.pcMask = (uint8_t)pc_mask,
		.secAttribute = CY_PPC_NON_SECURE,
		.privAttribute = CY_PPC_NONPRIV,
	};

	for (size_t i = 0; i < count; i++) {
		cy_en_prot_region_t reg = (cy_en_prot_region_t)regions[i];

		/*
		 * Log BEFORE the write.  The console SCB is itself one of these
		 * regions, so once it is made NS this message (and all later
		 * Secure output) may go silent — the last region logged identifies
		 * where the Secure console handed off to NS.
		 */
		LOG_DBG("ppc@%08x cfg region 0x%x", addr, (unsigned int)reg);

		cy_rslt_t r = Cy_Ppc_ConfigAttrib(base, reg, &cfg);

		if (r == CY_RSLT_SUCCESS) {
			r = Cy_Ppc_SetPcMask(base, reg, pc_mask);
		}
		if (r != CY_RSLT_SUCCESS) {
			LOG_ERR("ppc@%08x region 0x%x err=%d", addr, (unsigned int)reg, (int)r);
		}
	}
}

void ppc_configure_ns_all(void)
{
#define PPC_OPEN_NODE(ppc_id)                                                                      \
	{                                                                                          \
		static const uint32_t _regs_##ppc_id[] = DT_PROP(ppc_id, nonsecure_regions);       \
		ppc_open_regions((PPC_Type *)DT_REG_ADDR(ppc_id), DT_REG_ADDR(ppc_id),             \
				 _regs_##ppc_id, ARRAY_SIZE(_regs_##ppc_id),                       \
				 DT_PROP(ppc_id, pc_mask));                                        \
	}

	DT_FOREACH_STATUS_OKAY(infineon_mxs40_ppc, PPC_OPEN_NODE)
}
