/*
 * Copyright (c) 2016 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Definitions for IEEE 802.3, Section 2 MII compatible PHY transceivers
 */

#ifndef ZEPHYR_INCLUDE_NET_MII_H_
#define ZEPHYR_INCLUDE_NET_MII_H_

/**
 * @brief Ethernet MII (media independent interface) functions
 * @defgroup ethernet_mii Ethernet MII Support Functions
 * @ingroup ethernet
 * @{
 */

/* MII management registers */
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
/** MMD Access Control Register */
#define MII_MMD_ACR    0xd
/** MMD Access Address Data Register */
#define MII_MMD_AADR   0xe
/** Extended Status Register */
#define MII_ESTAT      0xf

/* Basic Mode Control Register (BMCR) bit definitions */
/** PHY reset */
#define MII_BMCR_RESET             (1 << 15)
/** enable loopback mode */
#define MII_BMCR_LOOPBACK          (1 << 14)
/** 10=1000Mbps 01=100Mbps; 00=10Mbps */
#define MII_BMCR_SPEED_LSB         (1 << 13)
/** Auto-Negotiation enable */
#define MII_BMCR_AUTONEG_ENABLE    (1 << 12)
/** power down mode */
#define MII_BMCR_POWER_DOWN        (1 << 11)
/** isolate electrically PHY from MII */
#define MII_BMCR_ISOLATE           (1 << 10)
/** restart auto-negotiation */
#define MII_BMCR_AUTONEG_RESTART   (1 << 9)
/** full duplex mode */
#define MII_BMCR_DUPLEX_MODE       (1 << 8)
/** 10=1000Mbps 01=100Mbps; 00=10Mbps */
#define MII_BMCR_SPEED_MSB         (1 << 6)
/** Link Speed Field */
#define   MII_BMCR_SPEED_MASK      (1 << 6 | 1 << 13)
/** select speed 10 Mb/s */
#define   MII_BMCR_SPEED_10        (0 << 6 | 0 << 13)
/** select speed 100 Mb/s */
#define   MII_BMCR_SPEED_100       (0 << 6 | 1 << 13)
/** select speed 1000 Mb/s */
#define   MII_BMCR_SPEED_1000      (1 << 6 | 0 << 13)

/* Basic Mode Status Register (BMSR) bit definitions */
/** 100BASE-T4 capable */
#define MII_BMSR_100BASE_T4        (1 << 15)
/** 100BASE-X full duplex capable */
#define MII_BMSR_100BASE_X_FULL    (1 << 14)
/** 100BASE-X half duplex capable */
#define MII_BMSR_100BASE_X_HALF    (1 << 13)
/** 10 Mb/s full duplex capable */
#define MII_BMSR_10_FULL           (1 << 12)
/** 10 Mb/s half duplex capable */
#define MII_BMSR_10_HALF           (1 << 11)
/** 100BASE-T2 full duplex capable */
#define MII_BMSR_100BASE_T2_FULL   (1 << 10)
/** 100BASE-T2 half duplex capable */
#define MII_BMSR_100BASE_T2_HALF   (1 << 9)
/** extend status information in reg 15 */
#define MII_BMSR_EXTEND_STATUS     (1 << 8)
/** PHY accepts management frames with preamble suppressed */
#define MII_BMSR_MF_PREAMB_SUPPR   (1 << 6)
/** Auto-negotiation process completed */
#define MII_BMSR_AUTONEG_COMPLETE  (1 << 5)
/** remote fault detected */
#define MII_BMSR_REMOTE_FAULT      (1 << 4)
/** PHY is able to perform Auto-Negotiation */
#define MII_BMSR_AUTONEG_ABILITY   (1 << 3)
/** link is up */
#define MII_BMSR_LINK_STATUS       (1 << 2)
/** jabber condition detected */
#define MII_BMSR_JABBER_DETECT     (1 << 1)
/** extended register capabilities */
#define MII_BMSR_EXTEND_CAPAB      (1 << 0)

/* Auto-negotiation Advertisement Register (ANAR) bit definitions */
/* Auto-negotiation Link Partner Ability Register (ANLPAR) bit definitions */
/** next page */
#define MII_ADVERTISE_NEXT_PAGE    (1 << 15)
/** link partner acknowledge response */
#define MII_ADVERTISE_LPACK        (1 << 14)
/** remote fault */
#define MII_ADVERTISE_REMOTE_FAULT (1 << 13)
/** try for asymmetric pause */
#define MII_ADVERTISE_ASYM_PAUSE   (1 << 11)
/** try for pause */
#define MII_ADVERTISE_PAUSE        (1 << 10)
/** try for 100BASE-T4 support */
#define MII_ADVERTISE_100BASE_T4   (1 << 9)
/** try for 100BASE-X full duplex support */
#define MII_ADVERTISE_100_FULL     (1 << 8)
/** try for 100BASE-X support */
#define MII_ADVERTISE_100_HALF     (1 << 7)
/** try for 10 Mb/s full duplex support */
#define MII_ADVERTISE_10_FULL      (1 << 6)
/** try for 10 Mb/s half duplex support */
#define MII_ADVERTISE_10_HALF      (1 << 5)
/** Selector Field */
#define MII_ADVERTISE_SEL_MASK     (0x1F << 0)
#define MII_ADVERTISE_SEL_IEEE_802_3   0x01

#define MII_ADVERTISE_ALL (MII_ADVERTISE_10_HALF | MII_ADVERTISE_10_FULL |\
			   MII_ADVERTISE_100_HALF | MII_ADVERTISE_100_FULL |\
			   MII_ADVERTISE_SEL_IEEE_802_3)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_MII_H_ */
