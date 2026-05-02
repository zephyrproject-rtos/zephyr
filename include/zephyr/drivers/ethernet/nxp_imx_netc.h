/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NXP_IMX_NETC_H__
#define __NXP_IMX_NETC_H__

#include <zephyr/net/dsa_core.h>
#include <zephyr/drivers/ptp_clock.h>

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

#if defined(CONFIG_NET_L2_PTP) && defined(CONFIG_PTP_CLOCK_NXP_NETC)
#define NETC_PTP_TIMESTAMPING_SUPPORT 1
#endif

#if defined(FSL_FEATURE_NETC_HAS_SWITCH_TAG) && FSL_FEATURE_NETC_HAS_SWITCH_TAG
#define NETC_SWITCH_TAG_SUPPORT 1
#endif

#if defined(CONFIG_DSA_NXP_IMX_NETC) && (!defined(NETC_SWITCH_TAG_SUPPORT))
#define NETC_SWITCH_NO_TAG_DRIVER_SUPPORT 1
#endif

#if defined(CONFIG_DSA_NXP_IMX_NETC)
/*
 * For tag supported switch, tag driver handles timestamping.
 * For no-tag supported switch, export timestamp functions for host driver to handle.
 */
#if defined(NETC_PTP_TIMESTAMPING_SUPPORT) && (!defined(NETC_SWITCH_TAG_SUPPORT))
void dsa_netc_port_txtsid(const struct device *dev, uint16_t id);
void dsa_netc_port_twostep_timestamp(struct dsa_switch_context *dsa_switch_ctx, uint16_t ts_req_id,
				     uint32_t timestamp);
#endif /* NETC_PTP_TIMESTAMPING_SUPPORT && NETC_SWITCH_TAG_SUPPORT */
#endif /* CONFIG_DSA_NXP_IMX_NETC */

#endif
