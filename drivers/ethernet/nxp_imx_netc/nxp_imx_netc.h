/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NXP_IMX_NETC_H__
#define __NXP_IMX_NETC_H__

#define NETC_BD_ALIGN 128

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

/* Get phy mode from dts. Default RMII for i.MXRT1180 ENETC which hasn't added the property. */
#define NETC_PHY_MODE(node_id)                                                                     \
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, mii)                                      \
		 ? kNETC_MiiMode                                                                   \
		 : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rmii)                          \
			    ? kNETC_RmiiMode                                                       \
			    : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii)              \
				       ? kNETC_RgmiiMode                                           \
				       : (DT_ENUM_HAS_VALUE(node_id, phy_connection_type, gmii)    \
						  ? kNETC_GmiiMode                                 \
						  : kNETC_RmiiMode))))

/* Helper macros to convert from Zephyr PHY speed to NETC speed/duplex types */
#define PHY_TO_NETC_SPEED(x)                                                                       \
	(PHY_LINK_IS_SPEED_1000M(x)                                                                \
		 ? kNETC_MiiSpeed1000M                                                             \
		 : (PHY_LINK_IS_SPEED_100M(x) ? kNETC_MiiSpeed100M : kNETC_MiiSpeed10M))

#define PHY_TO_NETC_DUPLEX_MODE(x)                                                                 \
	(PHY_LINK_IS_FULL_DUPLEX(x) ? kNETC_MiiFullDuplex : kNETC_MiiHalfDuplex)

#endif
