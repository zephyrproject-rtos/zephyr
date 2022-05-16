/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_KSZ8794_H__
#define __DSA_KSZ8794_H__

/* SPI commands */
#define KSZ8794_SPI_CMD_WR (BIT(6))
#define KSZ8794_SPI_CMD_RD (BIT(6) | BIT(5))

/* PHY registers */
#define KSZ8794_BMCR                                 0x00
#define KSZ8794_BMSR                                 0x01
#define KSZ8794_PHYID1                               0x02
#define KSZ8794_PHYID2                               0x03
#define KSZ8794_ANAR                                 0x04
#define KSZ8794_ANLPAR                               0x05
#define KSZ8794_LINKMD                               0x1D
#define KSZ8794_PHYSCS                               0x1F

/* SWITCH registers */
#define KSZ8794_CHIP_ID0                             0x00
#define KSZ8794_CHIP_ID1                             0x01
#define KSZ8794_GLOBAL_CTRL0                         0x02
#define KSZ8794_GLOBAL_CTRL1                         0x03
#define KSZ8794_GLOBAL_CTRL2                         0x04
#define KSZ8794_GLOBAL_CTRL3                         0x05
#define KSZ8794_GLOBAL_CTRL4                         0x06
#define KSZ8794_GLOBAL_CTRL5                         0x07
#define KSZ8794_GLOBAL_CTRL6_MIB_CTRL                0x08
#define KSZ8794_GLOBAL_CTRL7                         0x09
#define KSZ8794_GLOBAL_CTRL8                         0x0A
#define KSZ8794_GLOBAL_CTRL9                         0x0B
#define KSZ8794_GLOBAL_CTRL10                        0x0C
#define KSZ8794_GLOBAL_CTRL11                        0x0D
#define KSZ8794_PD_MGMT_CTRL1                        0x0E
#define KSZ8794_PD_MGMT_CTRL2                        0x0F
#define KSZ8794_PORT1_CTRL0                          0x10
#define KSZ8794_PORT1_CTRL1                          0x11
#define KSZ8794_PORT1_CTRL2                          0x12
#define KSZ8794_PORT1_CTRL3                          0x13
#define KSZ8794_PORT1_CTRL4                          0x14
#define KSZ8794_PORT1_CTRL5                          0x15
#define KSZ8794_PORT1_CTRL7                          0x17
#define KSZ8794_PORT1_STAT0                          0x18
#define KSZ8794_PORT1_STAT1                          0x19
#define KSZ8794_PORT1_PHY_CTRL8                      0x1A
#define KSZ8794_PORT1_LINKMD                         0x1B
#define KSZ8794_PORT1_PHY_CTRL9                      0x1C
#define KSZ8794_PORT1_PHY_CTRL10                     0x1D
#define KSZ8794_PORT1_STAT2                          0x1E
#define KSZ8794_PORT1_CTRL11_STAT3                   0x1F
#define KSZ8794_PORT2_CTRL0                          0x20
#define KSZ8794_PORT2_CTRL1                          0x21
#define KSZ8794_PORT2_CTRL2                          0x22
#define KSZ8794_PORT2_CTRL3                          0x23
#define KSZ8794_PORT2_CTRL4                          0x24
#define KSZ8794_PORT2_CTRL5                          0x25
#define KSZ8794_PORT2_CTRL7                          0x27
#define KSZ8794_PORT2_STAT0                          0x28
#define KSZ8794_PORT2_STAT1                          0x29
#define KSZ8794_PORT2_PHY_CTRL8                      0x2A
#define KSZ8794_PORT2_LINKMD                         0x2B
#define KSZ8794_PORT2_PHY_CTRL9                      0x2C
#define KSZ8794_PORT2_PHY_CTRL10                     0x2D
#define KSZ8794_PORT2_STAT2                          0x2E
#define KSZ8794_PORT2_CTRL11_STAT3                   0x2F
#define KSZ8794_PORT3_CTRL0                          0x30
#define KSZ8794_PORT3_CTRL1                          0x31
#define KSZ8794_PORT3_CTRL2                          0x32
#define KSZ8794_PORT3_CTRL3                          0x33
#define KSZ8794_PORT3_CTRL4                          0x34
#define KSZ8794_PORT3_CTRL5                          0x35
#define KSZ8794_PORT3_CTRL7                          0x37
#define KSZ8794_PORT3_STAT0                          0x38
#define KSZ8794_PORT3_STAT1                          0x39
#define KSZ8794_PORT3_PHY_CTRL8                      0x3A
#define KSZ8794_PORT3_LINKMD                         0x3B
#define KSZ8794_PORT3_PHY_CTRL9                      0x3C
#define KSZ8794_PORT3_PHY_CTRL10                     0x3D
#define KSZ8794_PORT3_STAT2                          0x3E
#define KSZ8794_PORT3_CTRL11_STAT3                   0x3F
#define KSZ8794_PORT4_CTRL0                          0x50
#define KSZ8794_PORT4_CTRL1                          0x51
#define KSZ8794_PORT4_CTRL2                          0x52
#define KSZ8794_PORT4_CTRL3                          0x53
#define KSZ8794_PORT4_CTRL4                          0x54
#define KSZ8794_PORT4_CTRL5                          0x55
#define KSZ8794_PORT4_IF_CTRL6                       0x56
#define KSZ8794_MAC_ADDR0                            0x68
#define KSZ8794_MAC_ADDR1                            0x69
#define KSZ8794_MAC_ADDR2                            0x6A
#define KSZ8794_MAC_ADDR3                            0x6B
#define KSZ8794_MAC_ADDR4                            0x6C
#define KSZ8794_MAC_ADDR5                            0x6D
#define KSZ8794_IND_ACCESS_CTRL0                     0x6E
#define KSZ8794_IND_ACCESS_CTRL1                     0x6F
#define KSZ8794_IND_DATA8                            0x70
#define KSZ8794_IND_DATA7                            0x71
#define KSZ8794_IND_DATA6                            0x72
#define KSZ8794_IND_DATA5                            0x73
#define KSZ8794_IND_DATA4                            0x74
#define KSZ8794_IND_DATA3                            0x75
#define KSZ8794_IND_DATA2                            0x76
#define KSZ8794_IND_DATA1                            0x77
#define KSZ8794_IND_DATA0                            0x78
#define KSZ8794_INT_STAT                             0x7C
#define KSZ8794_INT_MASK                             0x7D
#define KSZ8794_ACL_INT_STAT                         0x7E
#define KSZ8794_ACL_CTRL                             0x7F
#define KSZ8794_GLOBAL_CTRL12                        0x80
#define KSZ8794_GLOBAL_CTRL13                        0x81
#define KSZ8794_GLOBAL_CTRL14                        0x82
#define KSZ8794_GLOBAL_CTRL15                        0x83
#define KSZ8794_GLOBAL_CTRL16                        0x84
#define KSZ8794_GLOBAL_CTRL17                        0x85
#define KSZ8794_GLOBAL_CTRL18                        0x86
#define KSZ8794_GLOBAL_CTRL19                        0x87
#define KSZ8794_TOS_PRIO_CTRL0                       0x90
#define KSZ8794_TOS_PRIO_CTRL1                       0x91
#define KSZ8794_TOS_PRIO_CTRL2                       0x92
#define KSZ8794_TOS_PRIO_CTRL3                       0x93
#define KSZ8794_TOS_PRIO_CTRL4                       0x94
#define KSZ8794_TOS_PRIO_CTRL5                       0x95
#define KSZ8794_TOS_PRIO_CTRL6                       0x96
#define KSZ8794_TOS_PRIO_CTRL7                       0x97
#define KSZ8794_TOS_PRIO_CTRL8                       0x98
#define KSZ8794_TOS_PRIO_CTRL9                       0x99
#define KSZ8794_TOS_PRIO_CTRL10                      0x9A
#define KSZ8794_TOS_PRIO_CTRL11                      0x9B
#define KSZ8794_TOS_PRIO_CTRL12                      0x9C
#define KSZ8794_TOS_PRIO_CTRL13                      0x9D
#define KSZ8794_TOS_PRIO_CTRL14                      0x9E
#define KSZ8794_TOS_PRIO_CTRL15                      0x9F
#define KSZ8794_IND_BYTE                             0xA0
#define KSZ8794_GLOBAL_CTRL20                        0xA3
#define KSZ8794_GLOBAL_CTRL21                        0xA4
#define KSZ8794_PORT1_CTRL12                         0xB0
#define KSZ8794_PORT1_CTRL13                         0xB1
#define KSZ8794_PORT1_CTRL14                         0xB2
#define KSZ8794_PORT1_CTRL15                         0xB3
#define KSZ8794_PORT1_CTRL16                         0xB4
#define KSZ8794_PORT1_CTRL17                         0xB5
#define KSZ8794_PORT1_RATE_LIMIT_CTRL                0xB6
#define KSZ8794_PORT1_PRIO0_IG_LIMIT_CTRL1           0xB7
#define KSZ8794_PORT1_PRIO1_IG_LIMIT_CTRL2           0xB8
#define KSZ8794_PORT1_PRIO2_IG_LIMIT_CTRL3           0xB9
#define KSZ8794_PORT1_PRIO3_IG_LIMIT_CTRL4           0xBA
#define KSZ8794_PORT1_QUEUE0_EG_LIMIT_CTRL1          0xBB
#define KSZ8794_PORT1_QUEUE1_EG_LIMIT_CTRL2          0xBC
#define KSZ8794_PORT1_QUEUE2_EG_LIMIT_CTRL3          0xBD
#define KSZ8794_PORT1_QUEUE3_EG_LIMIT_CTRL4          0xBE
#define KSZ8794_TEST                                 0xBF
#define KSZ8794_PORT2_CTRL12                         0xC0
#define KSZ8794_PORT2_CTRL13                         0xC1
#define KSZ8794_PORT2_CTRL14                         0xC2
#define KSZ8794_PORT2_CTRL15                         0xC3
#define KSZ8794_PORT2_CTRL16                         0xC4
#define KSZ8794_PORT2_CTRL17                         0xC5
#define KSZ8794_PORT2_RATE_LIMIT_CTRL                0xC6
#define KSZ8794_PORT2_PRIO0_IG_LIMIT_CTRL1           0xC7
#define KSZ8794_PORT2_PRIO1_IG_LIMIT_CTRL2           0xC8
#define KSZ8794_PORT2_PRIO2_IG_LIMIT_CTRL3           0xC9
#define KSZ8794_PORT2_PRIO3_IG_LIMIT_CTRL4           0xCA
#define KSZ8794_PORT2_QUEUE0_EG_LIMIT_CTRL1          0xCB
#define KSZ8794_PORT2_QUEUE1_EG_LIMIT_CTRL2          0xCC
#define KSZ8794_PORT2_QUEUE2_EG_LIMIT_CTRL3          0xCD
#define KSZ8794_PORT2_QUEUE3_EG_LIMIT_CTRL4          0xCE
#define KSZ8794_PORT3_CTRL12                         0xD0
#define KSZ8794_PORT3_CTRL13                         0xD1
#define KSZ8794_PORT3_CTRL14                         0xD2
#define KSZ8794_PORT3_CTRL15                         0xD3
#define KSZ8794_PORT3_CTRL16                         0xD4
#define KSZ8794_PORT3_CTRL17                         0xD5
#define KSZ8794_PORT3_RATE_LIMIT_CTRL                0xD6
#define KSZ8794_PORT3_PRIO0_IG_LIMIT_CTRL1           0xD7
#define KSZ8794_PORT3_PRIO1_IG_LIMIT_CTRL2           0xD8
#define KSZ8794_PORT3_PRIO2_IG_LIMIT_CTRL3           0xD9
#define KSZ8794_PORT3_PRIO3_IG_LIMIT_CTRL4           0xDA
#define KSZ8794_PORT3_QUEUE0_EG_LIMIT_CTRL1          0xDB
#define KSZ8794_PORT3_QUEUE1_EG_LIMIT_CTRL2          0xDC
#define KSZ8794_PORT3_QUEUE2_EG_LIMIT_CTRL3          0xDD
#define KSZ8794_PORT3_QUEUE3_EG_LIMIT_CTRL4          0xDE
#define KSZ8794_TEST2                                0xDF
#define KSZ8794_TEST3                                0xEF
#define KSZ8794_PORT4_CTRL12                         0xF0
#define KSZ8794_PORT4_CTRL13                         0xF1
#define KSZ8794_PORT4_CTRL14                         0xF2
#define KSZ8794_PORT4_CTRL15                         0xF3
#define KSZ8794_PORT4_CTRL16                         0xF4
#define KSZ8794_PORT4_CTRL17                         0xF5
#define KSZ8794_PORT4_RATE_LIMIT_CTRL                0xF6
#define KSZ8794_PORT4_PRIO0_IG_LIMIT_CTRL1           0xF7
#define KSZ8794_PORT4_PRIO1_IG_LIMIT_CTRL2           0xF8
#define KSZ8794_PORT4_PRIO2_IG_LIMIT_CTRL3           0xF9
#define KSZ8794_PORT4_PRIO3_IG_LIMIT_CTRL4           0xFA
#define KSZ8794_PORT4_QUEUE0_EG_LIMIT_CTRL1          0xFB
#define KSZ8794_PORT4_QUEUE1_EG_LIMIT_CTRL2          0xFC
#define KSZ8794_PORT4_QUEUE2_EG_LIMIT_CTRL3          0xFD
#define KSZ8794_PORT4_QUEUE3_EG_LIMIT_CTRL4          0xFE
#define KSZ8794_TEST4                                0xFF

/* Basic Control register */
#define KSZ8794_BMCR_RESET                           0x8000
#define KSZ8794_BMCR_LOOPBACK                        0x4000
#define KSZ8794_BMCR_FORCE_100                       0x2000
#define KSZ8794_BMCR_AN_EN                           0x1000
#define KSZ8794_BMCR_POWER_DOWN                      0x0800
#define KSZ8794_BMCR_ISOLATE                         0x0400
#define KSZ8794_BMCR_RESTART_AN                      0x0200
#define KSZ8794_BMCR_FORCE_FULL_DUPLEX               0x0100
#define KSZ8794_BMCR_HP_MDIX                         0x0020
#define KSZ8794_BMCR_FORCE_MDI                       0x0010
#define KSZ8794_BMCR_AUTO_MDIX_DIS                   0x0008
#define KSZ8794_BMCR_FAR_END_FAULT_DIS               0x0004
#define KSZ8794_BMCR_TRANSMIT_DIS                    0x0002
#define KSZ8794_BMCR_LED_DIS                         0x0001

/* Basic Status register */
#define KSZ8794_BMSR_100BT4                          0x8000
#define KSZ8794_BMSR_100BTX_FD                       0x4000
#define KSZ8794_BMSR_100BTX_HD                       0x2000
#define KSZ8794_BMSR_10BT_FD                         0x1000
#define KSZ8794_BMSR_10BT_HD                         0x0800
#define KSZ8794_BMSR_AN_COMPLETE                     0x0020
#define KSZ8794_BMSR_FAR_END_FAULT                   0x0010
#define KSZ8794_BMSR_AN_CAPABLE                      0x0008
#define KSZ8794_BMSR_LINK_STATUS                     0x0004
#define KSZ8794_BMSR_EXTENDED_CAPABLE                0x0001

#define KSZ8794_GLOBAL_CTRL10_TAIL_TAG_EN            BIT(1)
#define KSZ8794_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_DIS BIT(1)

#define KSZ8794_CTRL2_PORTn(n)                       (0x12 + ((n) * 0x10))
#define KSZ8794_CTRL2_TRANSMIT_EN                    BIT(2)
#define KSZ8794_CTRL2_RECEIVE_EN                     BIT(1)
#define KSZ8794_CTRL2_LEARNING_DIS                   BIT(0)

#define KSZ8794_STAT2_PORTn(n)                       (0x1E + ((n) * 0x10))
#define KSZ8794_STAT2_LINK_GOOD                      BIT(5)

#define KSZ8794_CHIP_ID0_ID_DEFAULT                  0x87
#define KSZ8794_CHIP_ID1_ID_DEFAULT                  0x61
#define KSZ8794_PWR_MGNT_MODE_SOFT_DOWN              BIT(4)

#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_MASK          0x07
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_2MA           0x00
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_4MA           0x01
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_8MA           0x02
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_12MA          0x03
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_16MA          0x04
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_20MA          0x05
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_24MA          0x06
#define KSZ8794_GLOBAL_CTRL20_LOWSPEED_28MA          0x07

enum {
	/*
	 * KSZ8794 register's MAP
	 * (0x00 - 0x0F): Global Registers
	 * Port registers (offsets):
	 * (0x10): Port 1
	 * (0x20): Port 2
	 * (0x30): Port 3
	 * (0x40): Reserved
	 * (0x50): Port 4
	 */
	/* LAN ports for the ksz8794 switch */
	KSZ8794_PORT1 = 0,
	KSZ8794_PORT2,
	KSZ8794_PORT3,
	/*
	 * SWITCH <-> CPU port
	 *
	 * We also need to consider the "Reserved' offset
	 * defined above.
	 */
	KSZ8794_PORT4 = 4,
};

#define KSZ8794_REG_IND_DATA_8                        0x70
#define KSZ8794_REG_IND_DATA_7                        0x71
#define KSZ8794_REG_IND_DATA_6                        0x72
#define KSZ8794_REG_IND_DATA_5                        0x73
#define KSZ8794_REG_IND_DATA_4                        0x74
#define KSZ8794_REG_IND_DATA_3                        0x75
#define KSZ8794_REG_IND_DATA_2                        0x76
#define KSZ8794_REG_IND_DATA_1                        0x77
#define KSZ8794_REG_IND_DATA_0                        0x78

#define KSZ8794_REG_IND_CTRL_0                        0x6E
#define KSZ8794_REG_IND_CTRL_1                        0x6F

#define KSZ8794_STATIC_MAC_TABLE_VALID                BIT(5)
#define KSZ8794_STATIC_MAC_TABLE_OVRD                 BIT(6)

#define KSZ8XXX_CHIP_ID0                        KSZ8794_CHIP_ID0
#define KSZ8XXX_CHIP_ID1                        KSZ8794_CHIP_ID1
#define KSZ8XXX_CHIP_ID0_ID_DEFAULT             KSZ8794_CHIP_ID0_ID_DEFAULT
#define KSZ8XXX_CHIP_ID1_ID_DEFAULT             KSZ8794_CHIP_ID1_ID_DEFAULT
#define KSZ8XXX_FIRST_PORT                      KSZ8794_PORT1
#define KSZ8XXX_LAST_PORT                       KSZ8794_PORT3
#define KSZ8XXX_CPU_PORT                        KSZ8794_PORT4
#define KSZ8XXX_REG_IND_CTRL_0                  KSZ8794_REG_IND_CTRL_0
#define KSZ8XXX_REG_IND_CTRL_1                  KSZ8794_REG_IND_CTRL_1
#define KSZ8XXX_REG_IND_DATA_8                  KSZ8794_REG_IND_DATA_8
#define KSZ8XXX_REG_IND_DATA_7                  KSZ8794_REG_IND_DATA_7
#define KSZ8XXX_REG_IND_DATA_6                  KSZ8794_REG_IND_DATA_6
#define KSZ8XXX_REG_IND_DATA_5                  KSZ8794_REG_IND_DATA_5
#define KSZ8XXX_REG_IND_DATA_4                  KSZ8794_REG_IND_DATA_4
#define KSZ8XXX_REG_IND_DATA_3                  KSZ8794_REG_IND_DATA_3
#define KSZ8XXX_REG_IND_DATA_2                  KSZ8794_REG_IND_DATA_2
#define KSZ8XXX_REG_IND_DATA_1                  KSZ8794_REG_IND_DATA_1
#define KSZ8XXX_REG_IND_DATA_0                  KSZ8794_REG_IND_DATA_0
#define KSZ8XXX_STATIC_MAC_TABLE_VALID          KSZ8794_STATIC_MAC_TABLE_VALID
#define KSZ8XXX_STATIC_MAC_TABLE_OVRD           KSZ8794_STATIC_MAC_TABLE_OVRD
#define KSZ8XXX_STAT2_LINK_GOOD                 KSZ8794_STAT2_LINK_GOOD
#define KSZ8XXX_RESET_REG                       KSZ8794_PD_MGMT_CTRL1
#define KSZ8XXX_RESET_SET                       KSZ8794_PWR_MGNT_MODE_SOFT_DOWN
#define KSZ8XXX_RESET_CLEAR                     0
#define KSZ8XXX_STAT2_PORTn                     KSZ8794_STAT2_PORTn
#define KSZ8XXX_SPI_CMD_RD                      KSZ8794_SPI_CMD_RD
#define KSZ8XXX_SPI_CMD_WR                      KSZ8794_SPI_CMD_WR
#define KSZ8XXX_SOFT_RESET_DURATION                     1000
#define KSZ8XXX_HARD_RESET_WAIT                 10000
#endif /* __DSA_KSZ8794_H__ */
