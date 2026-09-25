/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @cond INTERNAL_HIDDEN */

#ifndef NRF_USBHS_PHY_CONFIG_H__
#define NRF_USBHS_PHY_CONFIG_H__

/* This macro works for both types, because differences between Type1 and Type2
 * are covered by devicetree bindings having different enum values and defaults.
 */
#define PHY_CONFIG_VALUE(node)                                                                     \
	(0 << USBHS_PHY_CONFIG_PLLITUNE_Pos)                                                      |\
	(0xC << USBHS_PHY_CONFIG_PLLPTUNE_Pos)                                                    |\
	(DT_ENUM_IDX(node, compdistune) << USBHS_PHY_CONFIG_COMPDISTUNE0_Pos)                     |\
	(DT_ENUM_IDX(node, sqrxtune) << USBHS_PHY_CONFIG_SQRXTUNE0_Pos)                           |\
	(DT_ENUM_IDX(node, vdatreftune) << USBHS_PHY_CONFIG_VDATREFTUNE0_Pos)                     |\
	((1 + DT_ENUM_IDX(node, txhsxvtune)) << USBHS_PHY_CONFIG_TXHSXVTUNE0_Pos)                 |\
	(BIT_MASK(DT_ENUM_IDX(node, txfslstune)) << USBHS_PHY_CONFIG_TXFSLSTUNE0_Pos)             |\
	(DT_ENUM_IDX(node, txvreftune) << USBHS_PHY_CONFIG_TXVREFTUNE0_Pos)                       |\
	(DT_ENUM_IDX(node, txrisetune) << USBHS_PHY_CONFIG_TXRISETUNE0_Pos)                       |\
	(DT_ENUM_IDX(node, txrestune) << USBHS_PHY_CONFIG_TXRESTUNE0_Pos)                         |\
	(DT_ENUM_IDX(node, txpreempamptune) << USBHS_PHY_CONFIG_TXPREEMPAMPTUNE0_Pos)             |\
	(DT_ENUM_IDX(node, txpreemppulsetune) << USBHS_PHY_CONFIG_TXPREEMPPULSETUNE0_Pos)

#define PHY_CONFIG(node)                                                                           \
	COND_CASE_1(DT_NODE_HAS_COMPAT(node, nordic_nrf_usbhs_phy_type1), (PHY_CONFIG_VALUE(node)),\
		    DT_NODE_HAS_COMPAT(node, nordic_nrf_usbhs_phy_type2), (PHY_CONFIG_VALUE(node)),\
		    (ZERO_OR_COMPILE_ERROR(0)))

#endif /* NRF_USBHS_PHY_CONFIG_H__ */

/** @endcond */
