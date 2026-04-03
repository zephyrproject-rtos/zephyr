/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define XEC_ECS_ETM_CR_OFS      0x1cu
#define XEC_ECS_ETM_PINS_EN_POS 0

#define XEC_ECS_DGB_CR_OFS             0x20u
#define XEC_ECS_DBG_CR_EN_POS          0
#define XEC_ECS_DBG_CR_PIN_CFG_POS     1
#define XEC_ECS_DBG_CR_PIN_CFG_MSK     GENMASK(2, 1)
#define XEC_ECS_DBG_CR_PIN_CFG_JTAG    0
#define XEC_ECS_DBG_CR_PIN_CFG_SWD_SWV 1
#define XEC_ECS_DBG_CR_PIN_CFG_SWD     2
#define XEC_ECS_DBG_CR_PIN_CFG_SET(c)  FIELD_PREP(XEC_ECS_DBG_CR_PIN_CFG_MSK, (c))
#define XEC_ECS_DBG_CR_PIN_CFG_GET(r)  FIELD_GET(XEC_ECS_DBG_CR_PIN_CFG_MSK, (r))
#define XEC_ECS_DBG_CR_PU_EN_POS       3
#define XEC_ECS_DBG_CR_EN_LOCK_POS     5

#define XEC_ECS_BASE_ADDR DT_REG_ADDR(DT_NODELABEL(ecs))

#define XEC_DEBUG_FLAG_EN_POS     0
#define XEC_DEBUG_FLAG_ETM_EN_POS 1
#define XEC_DEBUG_IFC_SWD_POS     2
#define XEC_DEBUG_SWD_SWV_POS     3
#define XEC_DEBUG_IFC_LOCK_POS    4

void mec_soc_debug_ifc_init(uint32_t flags)
{
	uint32_t msk = 0, val = 0;

	if ((flags & BIT(XEC_DEBUG_FLAG_ETM_EN_POS)) != 0) {
		/* Switch shared GPIOs to ETM mode */
		sys_set_bit(XEC_ECS_BASE_ADDR + XEC_ECS_ETM_CR_OFS, XEC_ECS_ETM_PINS_EN_POS);
	} else {
		/* Disable ETM. ETM pins can be used as GPIOs */
		sys_clear_bit(XEC_ECS_BASE_ADDR + XEC_ECS_ETM_CR_OFS, XEC_ECS_ETM_PINS_EN_POS);
	}

	if ((flags & BIT(XEC_DEBUG_FLAG_EN_POS)) != 0) {
		msk = BIT(XEC_ECS_DBG_CR_EN_POS) | XEC_ECS_DBG_CR_PIN_CFG_MSK;
		val = BIT(XEC_ECS_DBG_CR_EN_POS);

		if ((flags & BIT(XEC_DEBUG_IFC_LOCK_POS)) != 0) {
			msk |= BIT(XEC_ECS_DBG_CR_EN_LOCK_POS);
			val |= BIT(XEC_ECS_DBG_CR_EN_LOCK_POS);
		}

		if ((flags & BIT(XEC_DEBUG_IFC_SWD_POS)) != 0) {
			if ((flags & BIT(XEC_DEBUG_SWD_SWV_POS)) != 0) {
				val |= XEC_ECS_DBG_CR_PIN_CFG_SET(XEC_ECS_DBG_CR_PIN_CFG_SWD_SWV);
			} else {
				val |= XEC_ECS_DBG_CR_PIN_CFG_SET(XEC_ECS_DBG_CR_PIN_CFG_SWD);
			}
		} else {
			val |= XEC_ECS_DBG_CR_PIN_CFG_SET(XEC_ECS_DBG_CR_PIN_CFG_JTAG);
		}

		soc_mmcr_mask_set(XEC_ECS_BASE_ADDR + XEC_ECS_DGB_CR_OFS, val, msk);
	} else {
		sys_clear_bit(XEC_ECS_BASE_ADDR + XEC_ECS_DGB_CR_OFS, XEC_ECS_DBG_CR_EN_POS);
	}
}

int mec5_soc_common_init(void)
{
	uint32_t dbg_flags = 0; /* disabled */
	bool config_debug = false;

	/* Kconfig choice items. Only one will be defined */
#if defined(CONFIG_SOC_MEC_DEBUG_DISABLED)
	config_debug = true;
#endif
#if defined(SOC_MEC_DEBUG_WITHOUT_TRACING)
	config_debug = true;
	dbg_flags = BIT(XEC_DEBUG_FLAG_EN_POS) | BIT(XEC_DEBUG_IFC_SWD_POS);
#endif
#if defined(SOC_MEC_DEBUG_AND_TRACING)
	config_debug = true;
#if defined(CONFIG_SOC_MEC_DEBUG_AND_SWV_TRACING)
	dbg_flags = (BIT(XEC_DEBUG_FLAG_EN_POS) | BIT(XEC_DEBUG_IFC_SWD_POS) |
		     BIT(XEC_DEBUG_SWD_SWV_POS));
#endif
#if defined(config SOC_MEC_DEBUG_AND_ETM_TRACING)
	dbg_flags = (BIT(XEC_DEBUG_FLAG_EN_POS) | BIT(XEC_DEBUG_IFC_SWD_POS) |
		     BIT(XEC_DEBUG_FLAG_ETM_EN_POS));
#endif
#endif

	if (config_debug == true) {
		mec_soc_debug_ifc_init(dbg_flags);
	}

	soc_ecia_init(MCHP_MEC_ECIA_GIRQ_AGGR_ONLY_BM, MCHP_MEC_ECIA_GIRQ_DIRECT_CAP_BM, 0);

	return 0;
}
