/*
 * Copyright (c) 2023 Meshium
 *               Aleksandr Senin <al@meshium.net>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_KSZ8463_H__
#define __DSA_KSZ8463_H__

/* SPI commands */
#define KSZ8463_SPI_CMD_WR (BIT(7))
#define KSZ8463_SPI_CMD_RD (0)

#define KSZ8463_REG_ADDR_HI_PART(x)	(((x) & 0x7FF) >> 4)
#define KSZ8463_REG_ADDR_LO_PART(x)	(((x) & 0x00C) << 4)
#define KSZ8463_SPI_BYTE_ENABLE(x)	(BIT(((x) & 0x3) + 2))

/* PHY registers */
#define KSZ8463_BMCR                                 0x00
#define KSZ8463_BMSR                                 0x01
#define KSZ8463_PHYID1                               0x02
#define KSZ8463_PHYID2                               0x03
#define KSZ8463_ANAR                                 0x04
#define KSZ8463_ANLPAR                               0x05
#define KSZ8463_LINKMD                               0x1D
#define KSZ8463_PHYSCS                               0x1F

/* SWITCH registers */
#define KSZ8463_CHIP_ID0                             0x01
#define KSZ8463_CHIP_ID1                             0x00
#define KSZ8463_GLOBAL_CTRL_1L			     0x02
#define KSZ8463_GLOBAL_CTRL_1H			     0x03
#define KSZ8463_GLOBAL_CTRL_2L			     0x04
#define KSZ8463_GLOBAL_CTRL_2H			     0x05
#define KSZ8463_GLOBAL_CTRL_3L			     0x06
#define KSZ8463_GLOBAL_CTRL_3H			     0x07
#define KSZ8463_GLOBAL_CTRL_6L			     0x0C
#define KSZ8463_GLOBAL_CTRL_6H			     0x0D
#define KSZ8463_GLOBAL_CTRL_7L			     0x0E
#define KSZ8463_GLOBAL_CTRL_7H			     0x0F
#define KSZ8463_GLOBAL_CTRL_8L			     0xAC
#define KSZ8463_GLOBAL_CTRL_8H			     0xAD
#define KSZ8463_GLOBAL_CTRL_9L			     0xAE
#define KSZ8463_GLOBAL_CTRL_9H			     0xAF

#define KSZ8463_CFGR_L				     0xD8

#define KSZ8463_DSP_CNTRL_6L			     0x734
#define KSZ8463_DSP_CNTRL_6H			     0x735

#define KSZ8463_GLOBAL_CTRL1_TAIL_TAG_EN             BIT(0)
#define KSZ8463_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_ENA BIT(1)

#define KSZ8463_CTRL2L_PORTn(n)                      (0x6E + ((n) * 0x18))
#define KSZ8463_CTRL2L_VLAN_PORTS_MASK		     0xF8
#define KSZ8463_CTRL2H_PORTn(n)                      (0x6F + ((n) * 0x18))
#define KSZ8463_CTRL2_TRANSMIT_EN                    BIT(2)
#define KSZ8463_CTRL2_RECEIVE_EN                     BIT(1)
#define KSZ8463_CTRL2_LEARNING_DIS                   BIT(0)

#define KSZ8463_STAT2_PORTn(n)                       (0x80 + ((n) * 0x18))
#define KSZ8463_STAT2_LINK_GOOD                      BIT(5)

#define KSZ8463_CHIP_ID0_ID_DEFAULT                  0x84
#define KSZ8463_CHIP_ID1_ID_DEFAULT                  0x43
#define KSZ8463F_CHIP_ID1_ID_DEFAULT                 0x53
#define KSZ8463_RESET_REG	                     0x127
#define KSZ8463_SOFTWARE_RESET_SET                   BIT(0)
#define KSZ8463_SOFTWARE_RESET_CLEAR                 0

#define KSZ8463_P2_COPPER_MODE			     BIT(7)
#define KSZ8463_P1_COPPER_MODE			     BIT(6)
#define KSZ8463_RECV_ADJ			     BIT(5)

enum {
	/* LAN ports for the ksz8463 switch */
	KSZ8463_PORT1 = 0,
	KSZ8463_PORT2,
	/* SWITCH <-> CPU port */
	KSZ8463_PORT3,
};

#define KSZ8463_REG_IND_CTRL_0                        0x31
#define KSZ8463_REG_IND_CTRL_1                        0x30
#define KSZ8463_REG_IND_DATA_8                        0x26
#define KSZ8463_REG_IND_DATA_7                        0x2B
#define KSZ8463_REG_IND_DATA_6                        0x2A
#define KSZ8463_REG_IND_DATA_5                        0x29
#define KSZ8463_REG_IND_DATA_4                        0x28
#define KSZ8463_REG_IND_DATA_3                        0x2F
#define KSZ8463_REG_IND_DATA_2                        0x2E
#define KSZ8463_REG_IND_DATA_1                        0x2D
#define KSZ8463_REG_IND_DATA_0                        0x2C

#define KSZ8463_STATIC_MAC_TABLE_VALID                BIT(3)
#define KSZ8463_STATIC_MAC_TABLE_OVRD                 BIT(4)
#define KSZ8463_STATIC_MAC_TABLE_USE_FID              BIT(5)

#define KSZ8XXX_CHIP_ID0                        KSZ8463_CHIP_ID0
#define KSZ8XXX_CHIP_ID1                        KSZ8463_CHIP_ID1
#define KSZ8XXX_CHIP_ID0_ID_DEFAULT             KSZ8463_CHIP_ID0_ID_DEFAULT
#define KSZ8XXX_CHIP_ID1_ID_DEFAULT             KSZ8463F_CHIP_ID1_ID_DEFAULT
#define KSZ8XXX_FIRST_PORT                      KSZ8463_PORT1
#define KSZ8XXX_LAST_PORT                       KSZ8463_PORT3
#define KSZ8XXX_CPU_PORT                        KSZ8463_PORT3
#define KSZ8XXX_REG_IND_CTRL_0                  KSZ8463_REG_IND_CTRL_0
#define KSZ8XXX_REG_IND_CTRL_1                  KSZ8463_REG_IND_CTRL_1
#define KSZ8XXX_REG_IND_DATA_8                  KSZ8463_REG_IND_DATA_8
#define KSZ8XXX_REG_IND_DATA_7                  KSZ8463_REG_IND_DATA_7
#define KSZ8XXX_REG_IND_DATA_6                  KSZ8463_REG_IND_DATA_6
#define KSZ8XXX_REG_IND_DATA_5                  KSZ8463_REG_IND_DATA_5
#define KSZ8XXX_REG_IND_DATA_4                  KSZ8463_REG_IND_DATA_4
#define KSZ8XXX_REG_IND_DATA_3                  KSZ8463_REG_IND_DATA_3
#define KSZ8XXX_REG_IND_DATA_2                  KSZ8463_REG_IND_DATA_2
#define KSZ8XXX_REG_IND_DATA_1                  KSZ8463_REG_IND_DATA_1
#define KSZ8XXX_REG_IND_DATA_0                  KSZ8463_REG_IND_DATA_0
#define KSZ8XXX_STATIC_MAC_TABLE_VALID          KSZ8463_STATIC_MAC_TABLE_VALID
#define KSZ8XXX_STATIC_MAC_TABLE_OVRD           KSZ8463_STATIC_MAC_TABLE_OVRD
#define KSZ8XXX_STAT2_LINK_GOOD                 KSZ8463_STAT2_LINK_GOOD
#define KSZ8XXX_RESET_REG                       KSZ8463_RESET_REG
#define KSZ8XXX_RESET_SET                       KSZ8463_SOFTWARE_RESET_SET
#define KSZ8XXX_RESET_CLEAR                     KSZ8463_SOFTWARE_RESET_CLEAR
#define KSZ8XXX_STAT2_PORTn                     KSZ8463_STAT2_PORTn
#define KSZ8XXX_CTRL1_PORTn			KSZ8463_CTRL2L_PORTn
#define KSZ8XXX_CTRL1_VLAN_PORTS_MASK		KSZ8463_CTRL2L_VLAN_PORTS_MASK
#define KSZ8XXX_SPI_CMD_RD                      KSZ8463_SPI_CMD_RD
#define KSZ8XXX_SPI_CMD_WR                      KSZ8463_SPI_CMD_WR
#define KSZ8XXX_SOFT_RESET_DURATION             1000
#define KSZ8XXX_HARD_RESET_WAIT                 10000

#endif /* __DSA_KSZ8463_H__ */
