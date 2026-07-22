/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mpc/mpc.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include "cy_mpc.h"
#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
/* Cy_SysEnableSOCMEM() drives the SOCMEM power domain, an Edge-only block. */
#include "system_edge.h"
#endif

LOG_MODULE_REGISTER(infineon_mxs40_mpc);

/*
 * clang-format off: the region/node config macros use IF_ENABLED() and
 * COND_CODE_0(), whose partial-expression arguments clang-format cannot parse
 * and reindents inconsistently.  Keep the hand alignment for these.
 */
/* clang-format off */

/*
 * Configure one (offset, size) region for every (pc, secure, access)
 * 3-tuple listed in the child node's pc-configs property.
 *
 * mpc_base is passed explicitly via DT_FOREACH_CHILD_VARGS so we don't
 * need DT_PARENT inside the macro.
 */
#define MPC_CONFIGURE_REGION(region_id, mpc_base)                             \
	{                                                                      \
		static const uint32_t _pcs_##region_id[] =                    \
			DT_PROP(region_id, pc_configs);                        \
		uint32_t _off = DT_REG_ADDR(region_id);                       \
		uint32_t _sz  = DT_REG_SIZE(region_id);                       \
		for (size_t _i = 0;                                            \
		     _i < ARRAY_SIZE(_pcs_##region_id);                       \
		     _i += 3) {                                                \
			cy_stc_mpc_rot_cfg_t _cfg = {                          \
				.pc     = _pcs_##region_id[_i],                \
				.secure = _pcs_##region_id[_i + 1],            \
				.access = _pcs_##region_id[_i + 2],            \
			};                                                     \
			cy_rslt_t _r = Cy_Mpc_ConfigRotMpcStruct(             \
				(MPC_Type *)(mpc_base), _off, _sz, &_cfg);     \
			if (_r != CY_RSLT_SUCCESS) {                           \
				LOG_ERR("mpc@%08x region@%05x err=%d",        \
					(unsigned int)(mpc_base),             \
					(unsigned int)_off, (int)_r);         \
			}                                                      \
		}                                                              \
		IF_ENABLED(DT_PROP(region_id, bus_nonsecure), (               \
			cy_stc_mpc_cfg_t _bcfg = {                             \
				.secure = CY_MPC_NON_SECURE,                   \
			};                                                     \
			cy_rslt_t _br = Cy_Mpc_ConfigMpcStruct(               \
				(MPC_Type *)(mpc_base), _off, _sz, &_bcfg);    \
			if (_br != CY_RSLT_SUCCESS) {                          \
				LOG_ERR("mpc@%08x region@%05x bus err=%d",    \
					(unsigned int)(mpc_base),             \
					(unsigned int)_off, (int)_br);        \
			}                                                      \
		))                                                             \
	}

/*
 * For each enabled MPC node:
 *   1. Set violation response if "bus-error" is requested.
 *   2. Enable SOCMEM power domain if required by this MPC.
 *   3. Unless skip-auto-config, configure all child regions.
 */
#define MPC_CONFIGURE_NODE(mpc_id)                                                   \
	{                                                                            \
		uintptr_t _base = (uintptr_t)DT_REG_ADDR(mpc_id);                   \
		ARG_UNUSED(_base);                                                   \
		IF_ENABLED(IS_EQ(DT_ENUM_IDX(mpc_id, violation_response), 1), (     \
			Cy_Mpc_SetViolationResponse((MPC_Type *)_base,               \
						    CY_MPC_BUS_ERR);                 \
		))                                                                   \
		IF_ENABLED(DT_PROP(mpc_id, infineon_enable_socmem), (               \
			Cy_SysEnableSOCMEM(true);                                    \
		))                                                                   \
		COND_CODE_0(DT_PROP(mpc_id, skip_auto_config),                      \
			(DT_FOREACH_CHILD_VARGS(mpc_id,                              \
						MPC_CONFIGURE_REGION, _base)),       \
			())                                                          \
	}

/* clang-format on */

void mpc_configure_all(void)
{
	DT_FOREACH_STATUS_OKAY(infineon_mxs40_mpc, MPC_CONFIGURE_NODE)
}
