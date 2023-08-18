/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for IEEE 802.3 management interface
 */

#ifndef ZEPHYR_INCLUDE_NET_MDIO_H_
#define ZEPHYR_INCLUDE_NET_MDIO_H_

/**
 * @brief Definitions for IEEE 802.3 management interface
 * @defgroup ethernet_mdio IEEE 802.3 management interface
 * @ingroup ethernet
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** MDIO transaction operation code */
enum mdio_opcode {
	/** IEEE 802.3 22.2.4.5.4 write operation */
	MDIO_OP_C22_WRITE = 1,

	/** IEEE 802.3 22.2.4.5.4 read operation */
	MDIO_OP_C22_READ = 2,

	/** IEEE 802.3 45.3.4 address operation */
	MDIO_OP_C45_ADDRESS = 0,

	/** IEEE 802.3 45.3.4 write operation */
	MDIO_OP_C45_WRITE = 1,

	/** IEEE 802.3 45.3.4 post-read-increment-address operation */
	MDIO_OP_C45_READ_INC = 2,

	/** IEEE 802.3 45.3.4 read operation */
	MDIO_OP_C45_READ = 3
};

/* MDIO Manageable Device addresses */
/** Physical Medium Attachment / Physical Medium Dependent */
#define MDIO_MMD_PMAPMD			0x01U
/** WAN Interface Sublayer */
#define MDIO_MMD_WIS			0x02U
/** Physical Coding Sublayer */
#define MDIO_MMD_PCS			0x03U
/** PHY Extender Sublayer */
#define MDIO_MMD_PHYXS			0x04U
/** DTE Extender Sublayer */
#define MDIO_MMD_DTEXS			0x05U
/** Transmission Convergence */
#define MDIO_MMD_TC			0x06U
/** Auto-negotiation */
#define MDIO_MMD_AN			0x07U
/** Separated PMA (1) */
#define MDIO_MMD_SEPARATED_PMA1		0x08U
/** Separated PMA (2) */
#define MDIO_MMD_SEPARATED_PMA2		0x09U
/** Separated PMA (3) */
#define MDIO_MMD_SEPARATED_PMA3		0x0AU
/** Separated PMA (4) */
#define MDIO_MMD_SEPARATED_PMA4		0x0BU
/** Clause 22 extension */
#define MDIO_MMD_C22EXT			0x1DU
/** Vendor Specific 1 */
#define MDIO_MMD_VENDOR_SPECIFIC1	0x1EU
/** Vendor Specific 2 */
#define MDIO_MMD_VENDOR_SPECIFIC2	0x1FU

/* MDIO generic registers */
/** Control 1 */
#define MDIO_CTRL1			0x0000U
/** Status 1 */
#define MDIO_STAT1			0x0001U
/** Device identifier (1) */
#define MDIO_DEVID1			0x0002U
/** Device identifier (2) */
#define MDIO_DEVID2			0x0003U
/** Speed ability */
#define MDIO_SPEED			0x0004U
/** Devices in package (1) */
#define MDIO_DEVS1			0x0005U
/** Devices in package (2) */
#define MDIO_DEVS2			0x0006U
/** Control 2 */
#define MDIO_CTRL2			0x0007U
/** Status 2 */
#define MDIO_STAT2			0x0008U
/** Package identifier (1) */
#define MDIO_PKGID1			0x000EU
/** Package identifier (2) */
#define MDIO_PKGID2			0x000FU


/* BASE-T1 registers */
/** BASE-T1 Auto-negotiation control */
#define MDIO_AN_T1_CTRL			0x0200U
/** BASE-T1 Auto-negotiation status */
#define MDIO_AN_T1_STAT			0x0201U
/** BASE-T1 Auto-negotiation advertisement register [15:0] */
#define MDIO_AN_T1_ADV_L		0x0202U
/** BASE-T1 Auto-negotiation advertisement register [31:16] */
#define MDIO_AN_T1_ADV_M		0x0203U
/** BASE-T1 Auto-negotiation advertisement register [47:32] */
#define MDIO_AN_T1_ADV_H		0x0204U

/* BASE-T1 Auto-negotiation Control register */
/** Auto-negotiation Restart */
#define MDIO_AN_T1_CTRL_RESTART		BIT(9)
/** Auto-negotiation Enable */
#define MDIO_AN_T1_CTRL_EN		BIT(12)

/* BASE-T1 Auto-negotiation Status register */
/** Link Status */
#define MDIO_AN_T1_STAT_LINK_STATUS	BIT(2)
/** Auto-negotiation Ability */
#define MDIO_AN_T1_STAT_ABLE		BIT(3)
/** Auto-negotiation Remote Fault */
#define MDIO_AN_T1_STAT_REMOTE_FAULT	BIT(4)
/** Auto-negotiation Complete */
#define MDIO_AN_T1_STAT_COMPLETE	BIT(5)
/** Page Received */
#define MDIO_AN_T1_STAT_PAGE_RX		BIT(6)

/* BASE-T1 Auto-negotiation Advertisement register [15:0] */
/** Pause Ability */
#define MDIO_AN_T1_ADV_L_PAUSE_CAP	BIT(10)
/** Pause Ability */
#define MDIO_AN_T1_ADV_L_PAUSE_ASYM	BIT(11)
/** Force Master/Slave Configuration */
#define MDIO_AN_T1_ADV_L_FORCE_MS	BIT(12)
/** Remote Fault */
#define MDIO_AN_T1_ADV_L_REMOTE_FAULT	BIT(13)
/** Acknowledge (ACK) */
#define MDIO_AN_T1_ADV_L_ACK		BIT(14)
/** Next Page Request */
#define MDIO_AN_T1_ADV_L_NEXT_PAGE_REQ	BIT(15)

/* BASE-T1 Auto-negotiation Advertisement register [31:16] */
/** 10BASE-T1L Ability */
#define MDIO_AN_T1_ADV_M_B10L		BIT(14)
/** Master/slave Configuration */
#define MDIO_AN_T1_ADV_M_MST		BIT(4)

/* BASE-T1 Auto-negotiation Advertisement register [47:32] */
/* 10BASE-T1L High Level Transmit Operating Mode Request */
#define MDIO_AN_T1_ADV_H_10L_TX_HI_REQ	BIT(12)
/* 10BASE-T1L High Level Transmit Operating Mode Ability */
#define MDIO_AN_T1_ADV_H_10L_TX_HI	BIT(13)


/* 10BASE-T1L registers */
/** 10BASE-T1L PMA control */
#define MDIO_PMA_B10L_CTRL		0x08F6U
/** 10BASE-T1L PMA status */
#define MDIO_PMA_B10L_STAT		0x08F7U
/** 10BASE-T1L PMA link status*/
#define MDIO_PMA_B10L_LINK_STAT		0x8302U
/** 10BASE-T1L PCS control */
#define MDIO_PCS_B10L_CTRL		0x08E6U
/** 10BASE-T1L PCS status */
#define MDIO_PCS_B10L_STAT		0x08E7U

/* 10BASE-T1L PMA control register */
/** 10BASE-T1L Transmit Disable Mode */
#define MDIO_PMA_B10L_CTRL_TX_DIS_MODE_EN		BIT(14)
/** 10BASE-T1L Transmit Voltage Amplitude Control */
#define MDIO_PMA_B10L_CTRL_TX_LVL_HI			BIT(12)
/** 10BASE-T1L EEE Enable */
#define MDIO_PMA_B10L_CTRL_EEE				BIT(10)
/** 10BASE-T1L PMA Loopback */
#define MDIO_PMA_B10L_CTRL_LB_PMA_LOC_EN		BIT(0)

/* 10BASE-T1L PMA status register */
/** 10BASE-T1L PMA receive link up */
#define MDIO_PMA_B10L_STAT_LINK				BIT(0)
/** 10BASE-T1L Fault condition detected */
#define MDIO_PMA_B10L_STAT_FAULT			BIT(1)
/** 10BASE-T1L Receive polarity is reversed */
#define MDIO_PMA_B10L_STAT_POLARITY			BIT(2)
/** 10BASE-T1L Able to detect fault on receive path */
#define MDIO_PMA_B10L_STAT_RECV_FAULT			BIT(9)
/** 10BASE-T1L PHY has EEE ability */
#define MDIO_PMA_B10L_STAT_EEE				BIT(10)
/** 10BASE-T1L PMA has low-power ability */
#define MDIO_PMA_B10L_STAT_LOW_POWER			BIT(11)
/** 10BASE-T1L PHY has 2.4 Vpp operating mode ability */
#define MDIO_PMA_B10L_STAT_2V4_ABLE			BIT(12)
/** 10BASE-T1L PHY has loopback ability */
#define MDIO_PMA_B10L_STAT_LB_ABLE			BIT(13)

/* 10BASE-T1L PMA link status*/
/** 10BASE-T1L Remote Receiver Status OK Latch Low */
#define MDIO_PMA_B10L_LINK_STAT_REM_RCVR_STAT_OK_LL	BIT(9)
/** 10BASE-T1L Remote Receiver Status OK */
#define MDIO_PMA_B10L_LINK_STAT_REM_RCVR_STAT_OK	BIT(8)
/** 10BASE-T1L Local Receiver Status OK */
#define MDIO_PMA_B10L_LINK_STAT_LOC_RCVR_STAT_OK_LL	BIT(7)
/** 10BASE-T1L Local Receiver Status OK */
#define MDIO_PMA_B10L_LINK_STAT_LOC_RCVR_STAT_OK	BIT(6)
/** 10BASE-T1L Descrambler Status OK Latch Low */
#define MDIO_PMA_B10L_LINK_STAT_DSCR_STAT_OK_LL		BIT(5)
/** 10BASE-T1L Descrambler Status OK */
#define MDIO_PMA_B10L_LINK_STAT_DSCR_STAT_OK		BIT(4)
/** 10BASE-T1L Link Status OK Latch Low */
#define MDIO_PMA_B10L_LINK_STAT_LINK_STAT_OK_LL		BIT(1)
/** 10BASE-T1L Link Status OK */
#define MDIO_PMA_B10L_LINK_STAT_LINK_STAT_OK		BIT(0)

/* 10BASE-T1L PCS control */
/** 10BASE-T1L PCS Loopback Enable */
#define MDIO_PCS_B10L_CTRL_LB_PCS_EN			BIT(14)

/* 10BASE-T1L PCS status */
/** 10BASE-T1L PCS Descrambler Status */
#define MDIO_PCS_B10L_STAT_DSCR_STAT_OK_LL		BIT(2)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_MDIO_H_ */
