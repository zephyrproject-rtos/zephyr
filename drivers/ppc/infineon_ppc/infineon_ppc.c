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

/* Peripheral protection region attributes, one bit per region. */
#define PPC_PC_MASK  0x1000U /* PC_MASK[region]: protection contexts allowed access */
#define PPC_NS_ATT   0x2000U /* bit set: region is Non-Secure */
#define PPC_S_P_ATT  0x2400U /* bit set: Secure access may be unprivileged */
#define PPC_NS_P_ATT 0x4000U /* bit set: Non-Secure access may be unprivileged */

#define PPC_REGIONS_PER_ATT 32U /* regions per NS_ATT/S_P_ATT/NS_P_ATT word */

/* Mark one region unprivileged Non-Secure (security/privilege attributes). */
static void infineon_ppc_config_attrib(uintptr_t base, uint32_t region)
{
	uint32_t idx = region / PPC_REGIONS_PER_ATT;
	uint32_t bit = BIT(region % PPC_REGIONS_PER_ATT);

	sys_set_bits(base + PPC_NS_ATT + (idx * 4U), bit);
	sys_set_bits(base + PPC_S_P_ATT + (idx * 4U), bit);
	sys_set_bits(base + PPC_NS_P_ATT + (idx * 4U), bit);
}

/*
 * Configure all region attributes first, then write the PC masks in a second
 * pass, matching the PDL flow (Cy_Ppc_ConfigAttrib for every region followed by
 * Cy_Ppc_SetPcMask).
 */
#define PPC_OPEN_NODE(ppc_id)                                                                      \
	{                                                                                          \
		static const uint32_t _regs[] = DT_PROP(ppc_id, nonsecure_regions);                \
		uintptr_t _base = (uintptr_t)DT_REG_ADDR(ppc_id);                                  \
		uint32_t _mask = DT_PROP(ppc_id, pc_mask);                                         \
		for (size_t _i = 0; _i < ARRAY_SIZE(_regs); _i++) {                                \
			infineon_ppc_config_attrib(_base, _regs[_i]);                              \
		}                                                                                  \
		for (size_t _i = 0; _i < ARRAY_SIZE(_regs); _i++) {                                \
			sys_write32(_mask, _base + PPC_PC_MASK + (_regs[_i] * 4U));                \
		}                                                                                  \
	}

void ppc_configure_ns_all(void)
{
	DT_FOREACH_STATUS_OKAY(infineon_ppc, PPC_OPEN_NODE)
}
