/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for IEEE 802.3, Section 2 MII compatible PHY transceivers
 * @ingroup ethernet_mii
 */

#ifndef ZEPHYR_INCLUDE_NET_MII_H_
#define ZEPHYR_INCLUDE_NET_MII_H_

#include <zephyr/sys/util_macro.h>

/**
 * @brief Ethernet MII (media independent interface) functions
 * @defgroup ethernet_mii Ethernet MII Support Functions
 * @since 1.7
 * @version 0.8.0
 * @ingroup ethernet
 * @{
 */

/**
 * @name MII management registers
 * @{
 */
/** Basic Mode Control Register */
#define MII_BMCR       0x0
/** Basic Mode Status Register */
#define MII_BMSR       0x1
/** PHY ID 1 Register */
#define MII_PHYID1R    0x2
/** PHY ID 2 Register */
#define MII_PHYID2R    0x3
/** Auto-Negotiation Advertisement Register */
#define MII_ANAR       0x4
/** Auto-Negotiation Link Partner Ability Reg */
#define MII_ANLPAR     0x5
/** Auto-Negotiation Expansion Register */
#define MII_ANER       0x6
/** Auto-Negotiation Next Page Transmit Register */
#define MII_ANNPTR     0x7
/** Auto-Negotiation Link Partner Received Next Page Reg */
#define MII_ANLPRNPR   0x8
/** 1000BASE-T Control Register */
#define MII_1KTCR      0x9
/** 1000BASE-T Status Register */
#define MII_1KSTSR     0xa
/** MMD Access Control Register */
#define MII_MMD_ACR    0xd
/** MMD Access Address Data Register */
#define MII_MMD_AADR   0xe
/** Extended Status Register */
#define MII_ESTAT      0xf
/** @} */

/**
 * @name Basic Mode Control Register (BMCR) bit definitions
 * @{
 */
/** PHY reset bit position */
#define MII_BMCR_RESET_BIT           15
/** Loopback mode bit position */
#define MII_BMCR_LOOPBACK_BIT        14
/** Speed selection LSB bit position */
#define MII_BMCR_SPEED_LSB_BIT       13
/** Auto-Negotiation enable bit position */
#define MII_BMCR_AUTONEG_ENABLE_BIT  12
/** Power-down bit position */
#define MII_BMCR_POWER_DOWN_BIT      11
/** Isolate bit position */
#define MII_BMCR_ISOLATE_BIT         10
/** Restart Auto-Negotiation bit position */
#define MII_BMCR_AUTONEG_RESTART_BIT 9
/** Duplex mode bit position */
#define MII_BMCR_DUPLEX_MODE_BIT     8
/** Speed selection MSB bit position */
#define MII_BMCR_SPEED_MSB_BIT       6
/** PHY reset */
#define MII_BMCR_RESET               BIT(MII_BMCR_RESET_BIT)
/** Enable loopback mode */
#define MII_BMCR_LOOPBACK            BIT(MII_BMCR_LOOPBACK_BIT)
/** 10=1000Mbps 01=100Mbps; 00=10Mbps */
#define MII_BMCR_SPEED_LSB           BIT(MII_BMCR_SPEED_LSB_BIT)
/** Auto-Negotiation enable */
#define MII_BMCR_AUTONEG_ENABLE      BIT(MII_BMCR_AUTONEG_ENABLE_BIT)
/** Power down mode */
#define MII_BMCR_POWER_DOWN          BIT(MII_BMCR_POWER_DOWN_BIT)
/** Isolate electrically PHY from MII */
#define MII_BMCR_ISOLATE             BIT(MII_BMCR_ISOLATE_BIT)
/** Restart auto-negotiation */
#define MII_BMCR_AUTONEG_RESTART     BIT(MII_BMCR_AUTONEG_RESTART_BIT)
/** Full duplex mode */
#define MII_BMCR_DUPLEX_MODE         BIT(MII_BMCR_DUPLEX_MODE_BIT)
/** 10=1000Mbps 01=100Mbps; 00=10Mbps */
#define MII_BMCR_SPEED_MSB           BIT(MII_BMCR_SPEED_MSB_BIT)
/** Link Speed Field */
#define MII_BMCR_SPEED_MASK          (MII_BMCR_SPEED_MSB | MII_BMCR_SPEED_LSB)
/** Select speed 10 Mb/s */
#define MII_BMCR_SPEED_10            0
/** Select speed 100 Mb/s */
#define MII_BMCR_SPEED_100           BIT(MII_BMCR_SPEED_LSB_BIT)
/** Select speed 1000 Mb/s */
#define MII_BMCR_SPEED_1000          BIT(MII_BMCR_SPEED_MSB_BIT)
/** @} */

/**
 * @name Basic Mode Status Register (BMSR) bit definitions
 * @{
 */
/** 100BASE-T4 capable bit position */
#define MII_BMSR_100BASE_T4_BIT       15
/** 100BASE-X full duplex capable bit position */
#define MII_BMSR_100BASE_X_FULL_BIT   14
/** 100BASE-X half duplex capable bit position */
#define MII_BMSR_100BASE_X_HALF_BIT   13
/** 10 Mb/s full duplex capable bit position */
#define MII_BMSR_10_FULL_BIT          12
/** 10 Mb/s half duplex capable bit position */
#define MII_BMSR_10_HALF_BIT          11
/** 100BASE-T2 full duplex capable bit position */
#define MII_BMSR_100BASE_T2_FULL_BIT  10
/** 100BASE-T2 half duplex capable bit position */
#define MII_BMSR_100BASE_T2_HALF_BIT  9
/** Extended status information bit position */
#define MII_BMSR_EXTEND_STATUS_BIT    8
/** Preamble suppression capable bit position */
#define MII_BMSR_MF_PREAMB_SUPPR_BIT  6
/** Auto-Negotiation complete bit position */
#define MII_BMSR_AUTONEG_COMPLETE_BIT 5
/** Remote fault bit position */
#define MII_BMSR_REMOTE_FAULT_BIT     4
/** Auto-Negotiation ability bit position */
#define MII_BMSR_AUTONEG_ABILITY_BIT  3
/** Link status bit position */
#define MII_BMSR_LINK_STATUS_BIT      2
/** Jabber detect bit position */
#define MII_BMSR_JABBER_DETECT_BIT    1
/** Extended capability bit position */
#define MII_BMSR_EXTEND_CAPAB_BIT     0
/** 100BASE-T4 capable */
#define MII_BMSR_100BASE_T4           BIT(MII_BMSR_100BASE_T4_BIT)
/** 100BASE-X full duplex capable */
#define MII_BMSR_100BASE_X_FULL       BIT(MII_BMSR_100BASE_X_FULL_BIT)
/** 100BASE-X half duplex capable */
#define MII_BMSR_100BASE_X_HALF       BIT(MII_BMSR_100BASE_X_HALF_BIT)
/** 10 Mb/s full duplex capable */
#define MII_BMSR_10_FULL              BIT(MII_BMSR_10_FULL_BIT)
/** 10 Mb/s half duplex capable */
#define MII_BMSR_10_HALF              BIT(MII_BMSR_10_HALF_BIT)
/** 100BASE-T2 full duplex capable */
#define MII_BMSR_100BASE_T2_FULL      BIT(MII_BMSR_100BASE_T2_FULL_BIT)
/** 100BASE-T2 half duplex capable */
#define MII_BMSR_100BASE_T2_HALF      BIT(MII_BMSR_100BASE_T2_HALF_BIT)
/** Extend status information in reg 15 */
#define MII_BMSR_EXTEND_STATUS        BIT(MII_BMSR_EXTEND_STATUS_BIT)
/** PHY accepts management frames with preamble suppressed */
#define MII_BMSR_MF_PREAMB_SUPPR      BIT(MII_BMSR_MF_PREAMB_SUPPR_BIT)
/** Auto-negotiation process completed */
#define MII_BMSR_AUTONEG_COMPLETE     BIT(MII_BMSR_AUTONEG_COMPLETE_BIT)
/** Remote fault detected */
#define MII_BMSR_REMOTE_FAULT         BIT(MII_BMSR_REMOTE_FAULT_BIT)
/** PHY is able to perform Auto-Negotiation */
#define MII_BMSR_AUTONEG_ABILITY      BIT(MII_BMSR_AUTONEG_ABILITY_BIT)
/** Link is up */
#define MII_BMSR_LINK_STATUS          BIT(MII_BMSR_LINK_STATUS_BIT)
/** Jabber condition detected */
#define MII_BMSR_JABBER_DETECT        BIT(MII_BMSR_JABBER_DETECT_BIT)
/** Extended register capabilities */
#define MII_BMSR_EXTEND_CAPAB         BIT(MII_BMSR_EXTEND_CAPAB_BIT)
/** @} */

/**
 * @name Auto-negotiation Advertisement (ANAR) and Link Partner Ability
 *       (ANLPAR) Register bit definitions
 * @{
 */
/** Next page bit position */
#define MII_ADVERTISE_NEXT_PAGE_BIT    15
/** Link partner acknowledge bit position */
#define MII_ADVERTISE_LPACK_BIT        14
/** Remote fault bit position */
#define MII_ADVERTISE_REMOTE_FAULT_BIT 13
/** Asymmetric pause bit position */
#define MII_ADVERTISE_ASYM_PAUSE_BIT   11
/** Pause bit position */
#define MII_ADVERTISE_PAUSE_BIT        10
/** 100BASE-T4 support bit position */
#define MII_ADVERTISE_100BASE_T4_BIT   9
/** 100BASE-X full duplex support bit position */
#define MII_ADVERTISE_100_FULL_BIT     8
/** 100BASE-X half duplex support bit position */
#define MII_ADVERTISE_100_HALF_BIT     7
/** 10 Mb/s full duplex support bit position */
#define MII_ADVERTISE_10_FULL_BIT      6
/** 10 Mb/s half duplex support bit position */
#define MII_ADVERTISE_10_HALF_BIT      5
/** Next page */
#define MII_ADVERTISE_NEXT_PAGE        BIT(MII_ADVERTISE_NEXT_PAGE_BIT)
/** Link partner acknowledge response */
#define MII_ADVERTISE_LPACK            BIT(MII_ADVERTISE_LPACK_BIT)
/** Remote fault */
#define MII_ADVERTISE_REMOTE_FAULT     BIT(MII_ADVERTISE_REMOTE_FAULT_BIT)
/** Try for asymmetric pause */
#define MII_ADVERTISE_ASYM_PAUSE       BIT(MII_ADVERTISE_ASYM_PAUSE_BIT)
/** Try for pause */
#define MII_ADVERTISE_PAUSE            BIT(MII_ADVERTISE_PAUSE_BIT)
/** Try for 100BASE-T4 support */
#define MII_ADVERTISE_100BASE_T4       BIT(MII_ADVERTISE_100BASE_T4_BIT)
/** Try for 100BASE-X full duplex support */
#define MII_ADVERTISE_100_FULL         BIT(MII_ADVERTISE_100_FULL_BIT)
/** Try for 100BASE-X support */
#define MII_ADVERTISE_100_HALF         BIT(MII_ADVERTISE_100_HALF_BIT)
/** Try for 10 Mb/s full duplex support */
#define MII_ADVERTISE_10_FULL          BIT(MII_ADVERTISE_10_FULL_BIT)
/** Try for 10 Mb/s half duplex support */
#define MII_ADVERTISE_10_HALF          BIT(MII_ADVERTISE_10_HALF_BIT)
/** Selector Field Mask */
#define MII_ADVERTISE_SEL_MASK         (0x1F << 0)
/** Selector Field */
#define MII_ADVERTISE_SEL_IEEE_802_3   0x01
/** @} */

/**
 * @name 1000BASE-T Control Register bit definitions
 * @{
 */
/** 1000BASE-T full duplex support bit position */
#define MII_ADVERTISE_1000_FULL_BIT 9
/** 1000BASE-T half duplex support bit position */
#define MII_ADVERTISE_1000_HALF_BIT 8
/** Try for 1000BASE-T full duplex support */
#define MII_ADVERTISE_1000_FULL     BIT(MII_ADVERTISE_1000_FULL_BIT)
/** Try for 1000BASE-T half duplex support */
#define MII_ADVERTISE_1000_HALF     BIT(MII_ADVERTISE_1000_HALF_BIT)
/** @} */

/** Advertise all speeds */
#define MII_ADVERTISE_ALL (MII_ADVERTISE_10_HALF | MII_ADVERTISE_10_FULL |\
			   MII_ADVERTISE_100_HALF | MII_ADVERTISE_100_FULL |\
			   MII_ADVERTISE_SEL_IEEE_802_3)

/**
 * @name Extended Status Register bit definitions
 * @{
 */
/** 1000BASE-X full-duplex capable */
#define MII_ESTAT_1000BASE_X_FULL  BIT(15)
/** 1000BASE-X half-duplex capable */
#define MII_ESTAT_1000BASE_X_HALF  BIT(14)
/** 1000BASE-T full-duplex capable */
#define MII_ESTAT_1000BASE_T_FULL  BIT(13)
/** 1000BASE-T half-duplex capable */
#define MII_ESTAT_1000BASE_T_HALF  BIT(12)
/** @} */

/**
 * @name MMD Access Control Register (MII_MMD_ACR) bit definitions
 * @{
 */
/** DEVAD Mask */
#define MII_MMD_ACR_DEVAD_MASK      (0x1F << 0)
/** Address */
#define MII_MMD_ACR_ADDR            (0x00 << 14)
/** Data, no post increment */
#define MII_MMD_ACR_DATA_NO_POS_INC (0x01 << 14)
/** Data, post increment on reads and writes */
#define MII_MMD_ACR_DATA_RW_POS_INC (0x10 << 14)
/** Data, post increment on writes only */
#define MII_MMD_ACR_DATA_W_POS_INC  (0x11 << 14)
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_MII_H_ */
