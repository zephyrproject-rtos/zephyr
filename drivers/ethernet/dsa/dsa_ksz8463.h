/*
 * Copyright (c) 2026 Vilhelm Engström
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_DSA_KSZ8463_H_
#define ZEPHYR_DRIVERS_ETHERNET_DSA_KSZ8463_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

/* KSZ8463 family identifier */
#define KSZ8463_FAMILY_ID    0x84
/* Chip ID for KSZ8463ML and KSZ8463FML */
#define KSZ8463_MII_CHIP_ID  0x04
/* Chip ID for KSz8463RL and KSZ8463FRL */
#define KSZ8463_RMII_CHIP_ID 0x05

/* Number of ports */
#define KSZ8463_NUM_PORTS 3

/* Number of user ports */
#define KSZ8463_NUM_USER_PORTS (KSZ8463_NUM_PORTS - 1)

#define KSZ8463_SPEED_FULL_DUPLEX_BIT BIT(0)
#define KSZ8463_SPEED_100BASE_BIT     BIT(1)

/* Speeds supported by the KSZ8463, T4 omitted.
 *
 * Order should match the enum list for the
 * microchip,fixed-link-speed property
 */
enum ksz8463_speed {

	/* 10Mbps, half-duplex */
	KSZ8463_SPEED_10BASE_HALF_DUPLEX,

	/* 10Mbps, full-duplex */
	KSZ8463_SPEED_10BASE_FULL_DUPLEX = KSZ8463_SPEED_FULL_DUPLEX_BIT,

	/* 100Mbps, half-duplex */
	KSZ8463_SPEED_100BASE_HALF_DUPLEX = KSZ8463_SPEED_100BASE_BIT,

	/* 100Mbpx, full-duplex */
	KSZ8463_SPEED_100BASE_FULL_DUPLEX =
		KSZ8463_SPEED_FULL_DUPLEX_BIT | KSZ8463_SPEED_100BASE_BIT,

	KSZ8463_SPEED_AFTER_LAST_,
	KSZ8463_SPEED_MAX = KSZ8463_SPEED_AFTER_LAST_ - 1
};

/* Most significant bit in the first byte of an SPI transaction is
 * read/write desginator
 */
enum {
	/* Bit not set when reading */
	KSZ8463_SPI_CMD_RD = 0,
	/* Bit set when writing */
	KSZ8463_SPI_CMD_WR = BIT(7),
};

/* From Table 3-14 in data sheet */
enum {
	/* Size of command phase, in bytes */
	KSZ8463_SPI_CMD_PH_SIZE = 2,

	/* Offset of the data phase, in bytes */
	KSZ8463_SPI_CMD_DATA_PH_OFF = KSZ8463_SPI_CMD_PH_SIZE,

	/* Maximum size of the data phase */
	KSZ8463_SPI_CMD_MAX_DATA_PH_SIZE = 4,

	/* Max size of a complete SPI command */
	KSZ8463_SPI_CMD_MAX_SIZE = KSZ8463_SPI_CMD_PH_SIZE + KSZ8463_SPI_CMD_MAX_DATA_PH_SIZE,
};

/* PHY x and MII basic control register. 16-bit accesses */
#define KSZ8463_REG_PxMBCR(x) (0x4c + ((x) * 0x0c))

/* PHY x and MII basic control register, low byte. 8-bit access */
#define KSZ8463_REG_PxMBCR_LO(x) KSZ8463_REG_PxMBCR(x)

/* PHY x and MII basic control register, high byte. 8-bit access */
#define KSZ8463_REG_PxMBCR_HI(x) (KSZ8463_REG_PxMBCR_LO(x) + 1)

/* PHY x and MII basic status register. 16-bit access */
#define KSZ8463_REG_PxMBSR(x) (0x4e + ((x) * 0x0c))

/* PHY x and MII basic status register, low byte. 8-bit access */
#define KSZ8463_REG_PxMBSR_LO(x) KSZ8463_REG_PxMBSR(x)

/* PHY x and MII basic status register, high byte. 8-bit access */
#define KSZ8463_REG_PxMBSR_HI(x) (KSZ8463_REG_PxMBSR_LO(x) + 1)

/* PHY x auto-negotiation advertisement register, 16-bit access */
#define KSZ8463_REG_PxANAR(x) (0x54 + ((x) * 0x0c))

/* PHY x auto-negotiation advertisement register, low byte. 8-bit access */
#define KSZ8463_REG_PxANAR_LO(x) KSZ8463_REG_PxANAR(x)

/* PHY x auto-negotiation advertisement register,highlow byte. 8-bit access */
#define KSZ8463_REG_PxANAR_HI(x) (KSZ8463_REG_PxANAR_LO(x) + 1)

/* Port x control register 2. 16-bit access */
#define KSZ8463_REG_PxCR2(x) (0x6e + ((x) * 0x18))

/* Port x control register 2, low byte. 8-bit access */
#define KSZ8463_REG_PxCR2_LO(x) KSZ8463_REG_PxCR2(x)

/* Port x control register 2, high byte. 8-bit access */
#define KSZ8463_REG_PxCR2_HI(x) (KSZ8463_REG_PxCR2_LO(x) + 1)

/* Port x control register 4. 16-bit access */
#define KSZ8463_REG_PxCR4(x) (0x7e + ((x) * 0x18))

/* Port x control register 4, low byte. 8-bit access */
#define KSZ8463_REG_PxCR4_LO(x) KSZ8463_REG_PxCR4(x)

/* Port x control register 4, high byte. 8-bit access */
#define KSZ8463_REG_PxCR4_HI(x) (KSZ8463_REG_PxCR4_LO(x) + 1)

/* Port x status register. 16-bit access */
#define KSZ8463_REG_PxSR(x) (0x80 + ((x) * 0x18))

/* Port x status register, low byte. 8-bit access */
#define KSZ8463_REG_PxSR_LO(x) KSZ8463_REG_PxSR(x)

/* Port x status register, high byte. 8-bit access */
#define KSZ8463_REG_PxSR_HI(x) (KSZ8463_REG_PxSR_LO(x) + 1)

/* Port x EEE control/status and auto-negotiation expansion register. 16-bit
 * access
 */
#define KSZ8463_REG_PxEEECS(x) (0xe4 + ((x) * 0x0c))

/* Port x EEE contrl/status and auto-negotiation expansion register, low byte.
 * 8-bit access
 */
#define KSZ8463_REG_PxEEECS_LO(x) KSZ8463_REG_PxEEECS(x)

/* Port x EEE contrl/status and auto-negotiation expansion register, high byte.
 * 8-bit access
 */
#define KSZ8463_REG_PxEEECS_HI(x) (KSZ8463_REG_PxEEECS_LO(x) + 1)

/* Fixed-address registers used by the driver */
enum {
	/* Chip ID and enable register. 16-bit access */
	KSZ8463_REG_CIDER = 0x000,

	/* Switch global control register 2. 16-bit access */
	KSZ8463_REG_SGCR2 = 0x004,

	/* Switch global control register 2, low byte. 8-bit access */
	KSZ8463_REG_SGCR2_LO = KSZ8463_REG_SGCR2,

	/* Switch global control register 2, high byte. 8-bit access */
	KSZ8463_REG_SGCR2_HI = KSZ8463_REG_SGCR2_LO + 1,

	/* Switch global control register 7. 16-bit access */
	KSZ8463_REG_SGCR7 = 0x00e,

	/* Switch global control register 7, low byte. 8-bit access */
	KSZ8463_REG_SGCR7_LO = KSZ8463_REG_SGCR7,

	/* Switch global control register 7, high byte. 8-bit access */
	KSZ8463_REG_SGCR7_HI = KSZ8463_REG_SGCR7_LO + 1,

	/* Switch global control register 8. 16-bit access */
	KSZ8463_REG_SGCR8 = 0x0ac,

	/* Switch global control register 8, low byte. 8-bit access */
	KSZ8463_REG_SGCR8_LO = KSZ8463_REG_SGCR8,

	/* Switch global control register 8, high byte. 8-bit access */
	KSZ8463_REG_SGCR8_HI = KSZ8463_REG_SGCR8_LO + 1,

	/* PCS EEE control register. 8-bit access*/
	KSZ8463_REG_PCSEEEC = 0x0f3,

	/* Global reset register. 16-bit access */
	KSZ8463_REG_GRR = 0x126,

	/* Global reset register, low byte. 8-bit access */
	KSZ8463_REG_GRR_LO = KSZ8463_REG_GRR,

	/* Interrupt enable register. 16-bit access */
	KSZ8463_REG_IER = 0x190,

	/* Interrupt enable register, low byte. 8-bit access*/
	KSZ8463_REG_IER_LO = KSZ8463_REG_IER,

	/* Interrupt enable register, high byte. 8-bit access */
	KSZ8463_REG_IER_HI = KSZ8463_REG_IER_LO + 1,

	/* Interrupt status register. 16-bit access */
	KSZ8463_REG_ISR = 0x192,

	/* Interrupt status register, low byte. 8-bit access */
	KSZ8463_REG_ISR_LO = KSZ8463_REG_ISR,

	/* Interrupt status register, high byte. 8-bit access */
	KSZ8463_REG_ISR_HI = KSZ8463_REG_ISR_LO + 1
};

/* Global reset register */
enum {
	/* Set to enable global software reset */
	KSZ8463_GRR_GBL_SOFT_RST = BIT(0),
};

/* Interrupt enable register */
enum {
	/* Link change iterrupt enable */
	KSZ8463_IER_LCIE = BIT(15),

	/* Link change interrupt enable, index in high IER byte */
	KSZ8463_IER_HI_LCIE = KSZ8463_IER_LCIE >> 8,
};

/* Interrupt status register */
enum {
	/* Set if link status has changed. Write 1 to clear */
	KSZ8463_ISR_LCIS = BIT(15),

	/* Link change interrupt status, index in high byte. Write 1 to clear */
	KSZ8463_ISR_HI_LCIS = KSZ8463_ISR_LCIS >> 8,
};

/* PCS EEE control register */
enum {
	/* Port 2 next page enable */
	KSZ8463_PCSEEEC_P2_NEXT_PG_EN = BIT(1),

	/* Port 1 next page enable */
	KSZ8463_PCSEEEC_P1_NEXT_PG_EN = BIT(0),
};

/* Port x control register 2 */
enum {
	/* Set to enable packet transmission */
	KSZ8463_PxCR2_TX_EN = BIT(10),

	/* Set to enable packet reception */
	KSZ8463_PxCR2_RX_EN = BIT(9),

	/* Set to disable switch address learning */
	KSZ8463_PxCR2_LEARN_DIS = BIT(8),

	/* Set to enable packet transmission, index in high byte */
	KSZ8463_PxCR2_HI_TX_EN = KSZ8463_PxCR2_TX_EN >> 8,

	/* Set to enable packet reception, index in high byte */
	KSZ8463_PxCR2_HI_RX_EN = KSZ8463_PxCR2_RX_EN >> 8,

	/* Set to disable address learning, index in high byte */
	KSZ8463_PxCR2_HI_LEARN_DIS = KSZ8463_PxCR2_LEARN_DIS >> 8,
};

/* Port x control register 4 */
enum {
	/* Set to restart auto-negotiation */
	KSZ8463_PxCR4_AUTONEG_RESTART = BIT(13),

	/* Set to enable auto-negotiation */
	KSZ8463_PxCR4_AUTONEG_EN = BIT(7),

	/* Set to restart auto-negotiation. Index in high byte */
	KSZ8463_PxCR4_HI_AUTONEG_RESTART = KSZ8463_PxCR4_AUTONEG_RESTART >> 8,

	/* Set to enable auto-negotiation, index in low byte */
	KSZ8463_PxCR4_LO_AUTONEG_EN = KSZ8463_PxCR4_AUTONEG_EN,
};

/* Switch global control register 2 */
enum {
	/* Set to enable IGMP snooping */
	KSZ8463_SGCR2_IGMP_SNOOP_EN = BIT(14),

	/* Set to enable MLD snooping */
	KSZ8463_SGCR2_MLD_SNOOP_EN = BIT(13),

	/* Set to enable legal max packet size check */
	KSZ8463_SGCR2_LEGAL_PKT_SZ_CHK_EN = BIT(1),

	/* Set to enable IGMP snooping. Index in high byte */
	KSZ8463_SGCR2_HI_IGMP_SNOOP_EN = KSZ8463_SGCR2_IGMP_SNOOP_EN >> 8,

	/* Set to enable MLD snooping. Index in high byte */
	KSZ8463_SGCR2_HI_MLD_SNOOP_EN = KSZ8463_SGCR2_MLD_SNOOP_EN >> 8,

	/* Set to enable legal max packet size check. Index in low byte */
	KSZ8463_SGCR2_LO_LEGAL_PKT_SZ_CHK_EN = KSZ8463_SGCR2_LEGAL_PKT_SZ_CHK_EN,
};

/* Switch global control register 7 */
enum {
	/* LED1: speed, LED0: link and activity */
	KSZ8463_SGCR7_LED_MODE_SPD1_LNKACT0 = 0 << 8,

	/* LED1: activity, LED0: link */
	KSZ8463_SGCR7_LED_MODE_ACT1_LNK0 = 1 << 8,

	/* LED1: FUll-Duplex, LED0: link and activity */
	KSZ8463_SGCR7_LED_MODE_FDPLX1_LNKACT0 = 2 << 8,

	/* LED1: FUll-Duplex, LED0: link */
	KSZ8463_SGCR7_LED_MODE_FDPLX1_LNK0 = 3 << 8,

	/* LED mode bitmask */
	KSZ8463_SGCR7_PORT_LED_MODE_MASK = BIT_MASK(2) << 8,

	/* LED1: speed, LED0: link and activity. Index in high byte */
	KSZ8463_SGCR7_HI_LED_MODE_SPD1_LNKACT0 = KSZ8463_SGCR7_LED_MODE_SPD1_LNKACT0 >> 8,

	/* LED1: activity, LED0: link. Index in high byte */
	KSZ8463_SGCR7_HI_LED_MODE_ACT1_LNK0 = KSZ8463_SGCR7_LED_MODE_ACT1_LNK0 >> 8,

	/* LED1: FUll-Duplex, LED0: link and activity. Index in high byte */
	KSZ8463_SGCR7_HI_LED_MODE_FDPLX1_LNKACT0 = KSZ8463_SGCR7_LED_MODE_FDPLX1_LNKACT0 >> 8,

	/* LED1: FUll-Duplex, LED0: link. Index in high byte */
	KSZ8463_SGCR7_HI_LED_MODE_FDPLX1_LNK0 = KSZ8463_SGCR7_LED_MODE_FDPLX1_LNK0 >> 8,

	/* LED mode bitmask, bits in high byte */
	KSZ8463_SGCR7_HI_PORT_LED_MODE_MASK = KSZ8463_SGCR7_PORT_LED_MODE_MASK >> 8,
};

/* Switch global control register 8 */
enum {
	/* Set to enable tail tagging on port 3 */
	KSZ8463_SGCR8_TAIL_TAG_EN = BIT(8),

	/* Set to enable tail tagging on port 3, index in high byte */
	KSZ8463_SGCR8_HI_TAIL_TAG_EN = KSZ8463_SGCR8_TAIL_TAG_EN >> 8,
};

/* Port x status register */
enum {
	/* Set when link speed is 100Mbps, unset when 10Mbps */
	KSZ8463_PxSR_OPER_SPEED_100MBPS = BIT(10),

	/* Set then link duplex is full, unset when it's half */
	KSZ8463_PxSR_OPER_FULL_DUPLEX = BIT(9),

	/* Set when auto-negotiation has completed */
	KSZ8463_PxSR_AUTONEG_CPLT = BIT(6),

	/* Set when link status is good */
	KSZ8463_PxSR_LINK_STATUS = BIT(5),

	/* Set when link speed is 100Mbps, unset when 10Mbps. High byte index */
	KSZ8463_PxSR_HI_OPER_SPEED_100MBPS = KSZ8463_PxSR_OPER_SPEED_100MBPS >> 8,

	/* Set when link duplex is full, unset when half. Index in high byte */
	KSZ8463_PxSR_HI_OPER_FULL_DUPLEX = KSZ8463_PxSR_OPER_FULL_DUPLEX >> 8,

	/* Set when auto-negotiation has completed, index in low byte */
	KSZ8463_PxSR_LO_AUTONEG_CPLT = KSZ8463_PxSR_AUTONEG_CPLT,

	/* Set if link status is good, index in low byte */
	KSZ8463_PxSR_LO_LINK_STATUS = KSZ8463_PxSR_LINK_STATUS,
};

/* Port x EEE contrl/status and auto-negotiation expansion register */
enum {
	/* Set when link partner is auto-negotiation able */
	KSZ8463_PxEEECS_LNK_AUTONEG_CPBL = BIT(0),

	/* Set when link partner is auto-negotiation able, index in low byte */
	KSZ8463_PxEEECS_LO_LNK_AUTONEG_CPBL = KSZ8463_PxEEECS_LNK_AUTONEG_CPBL,
};

/* PHY x and MII basic control register */
enum {
	/* Set to force 100Mbps when auto-negotiation is disabled */
	KSZ8463_PxMBCR_FORCE_100BASE_TX = BIT(13),

	/* Set to force full-duplex when auto-negotiation is disabled */
	KSZ8463_PxMBCR_FORCE_FULL_DUPLEX = BIT(8),

	/* Force 100Mbps when auto-negotiation is disabled. High byte index */
	KSZ8463_PxMBCR_HI_FORCE_100BASE_TX = KSZ8463_PxMBCR_FORCE_100BASE_TX >> 8,

	/* Force full-duplex when auto-negotiation is disabled. High byte index */
	KSZ8463_PxMBCR_HI_FORCE_FULL_DUPLEX = KSZ8463_PxMBCR_FORCE_FULL_DUPLEX >> 8,
};

/* PHY x and MII basic status register */
enum {
	/* Set when auto-negotiation has completed */
	KSZ8463_PxMBSR_AUTONEG_CPLT = BIT(5),

	/* Link status, mirrored from KSZ8463_PxSR_LINK_STATUS */
	KSZ8463_PxMBSR_LINK_STATUS = BIT(2),

	/* Auto-negotiation complete, index in low byte */
	KSZ8463_PxMBSR_LO_AUTONEG_CPLT = KSZ8463_PxMBSR_AUTONEG_CPLT,

	/* Link status, index in low byte */
	KSZ8463_PxMBSR_LO_LINK_STATUS = KSZ8463_PxMBSR_LINK_STATUS,
};

/* PHY x auto-negotiation advertisement register */
enum {
	/* Advertise 100BASE-TX full-duplex */
	KSZ8463_PxANAR_ADV_100BASE_TX_FULL_DUPLEX = BIT(8),

	/* Advertise 100BASE-TX half-duplex */
	KSZ8463_PxANAR_ADV_100BASE_TX_HALF_DUPLEX = BIT(7),

	/* Advertise 10BASE-T full-duplex */
	KSZ8463_PxANAR_ADV_10BASE_T_FULL_DUPLEX = BIT(6),

	/* Advertise 10BASE-T half-duplex */
	KSZ8463_PxANAR_ADV_10BASE_T_HALF_DUPLEX = BIT(5),
};

#endif /* ZEPHYR_DRIVERS_ETHERNET_DSA_KSZ8463_H_ */
