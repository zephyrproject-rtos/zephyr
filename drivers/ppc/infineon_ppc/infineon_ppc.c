/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/ppc/ppc.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(infineon_ppc);

/*
 * Native register driver for the Infineon Peripheral Protection
 * Controller.  The PPC register block is identical across parts (PSOC
 * Edge, PSC3); only the base address, the peripheral protection-region indices
 * and the protection-context mask differ and come from the devicetree.
 * Programming the registers directly avoids the Cy_Ppc_* PDL, whose function
 * signatures differ between device families.
 *
 * Registers (offsets within the PPC block), each a per-region/per-PC bitmap:
 *   PC_MASK[region]  : protection-context (bus master) mask for one region
 *   NS_ATT[region/32]: bit set  -> region is Non-Secure
 *   NS_P_ATT[.]      : bit set  -> region is Non-Secure Non-Privileged
 *
 * Cy_Ppc_InitPpc()'s CTL write is deliberately not issued: Infineon silicon
 * restricts PPC CTL writes to the SROM protection context, so the
 * SROM-configured bus-error response is left in place.
 */
#define PPC_PC_MASK  0x1000U
#define PPC_NS_ATT   0x2000U
#define PPC_NS_P_ATT 0x4000U

#define PPC_REGIONS_PER_ATT 32U

/* Open one peripheral protection region to Non-Secure Non-Privileged access. */
static void infineon_ppc_open_region(uintptr_t base, uint32_t region, uint32_t pc_mask)
{
	uint32_t idx = region / PPC_REGIONS_PER_ATT;
	uint32_t bit = BIT(region % PPC_REGIONS_PER_ATT);

	sys_set_bits(base + PPC_NS_ATT + (idx * 4U), bit);
	sys_set_bits(base + PPC_NS_P_ATT + (idx * 4U), bit);
	sys_write32(pc_mask, base + PPC_PC_MASK + (region * 4U));
}

#define PPC_OPEN_NODE(ppc_id)                                                                      \
	{                                                                                          \
		static const uint32_t _regs[] = DT_PROP(ppc_id, nonsecure_regions);                \
		uintptr_t _base = (uintptr_t)DT_REG_ADDR(ppc_id);                                  \
		uint32_t _mask = DT_PROP(ppc_id, pc_mask);                                          \
		for (size_t _i = 0; _i < ARRAY_SIZE(_regs); _i++) {                                \
			infineon_ppc_open_region(_base, _regs[_i], _mask);                            \
		}                                                                                  \
	}

void ppc_configure_ns_all(void)
{
	DT_FOREACH_STATUS_OKAY(infineon_ppc, PPC_OPEN_NODE)
}
