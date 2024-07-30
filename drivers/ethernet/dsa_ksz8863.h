/*
 * Copyright (c) 2021 IP-Logix Inc.
 *               Arvin Farahmand <arvinf@ip-logix.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_KSZ8863_H__
#define __DSA_KSZ8863_H__

/* SPI commands */
#define KSZ8863_SPI_CMD_WR (BIT(6))
#define KSZ8863_SPI_CMD_RD (BIT(6) | BIT(5))

/* PHY registers */
#define KSZ8863_BMCR                                 0x00
#define KSZ8863_BMSR                                 0x01
#define KSZ8863_PHYID1                               0x02
#define KSZ8863_PHYID2                               0x03
#define KSZ8863_ANAR                                 0x04
#define KSZ8863_ANLPAR                               0x05
#define KSZ8863_LINKMD                               0x1D
#define KSZ8863_PHYSCS                               0x1F

/* SWITCH registers */
#define KSZ8863_CHIP_ID0                             0x00
#define KSZ8863_CHIP_ID1                             0x01
#define KSZ8863_GLOBAL_CTRL0                         0x02
#define KSZ8863_GLOBAL_CTRL1                         0x03
#define KSZ8863_GLOBAL_CTRL2                         0x04
#define KSZ8863_GLOBAL_CTRL3                         0x05
#define KSZ8863_GLOBAL_CTRL4                         0x06
#define KSZ8863_GLOBAL_CTRL5                         0x07
#define KSZ8863_GLOBAL_CTRL9                         0x0B
#define KSZ8863_GLOBAL_CTRL10                        0x0C
#define KSZ8863_GLOBAL_CTRL11                        0x0D
#define KSZ8863_GLOBAL_CTRL12                        0x0E
#define KSZ8863_GLOBAL_CTRL13                        0x0F
#define KSZ8863_PORT1_CTRL0                          0x10
#define KSZ8863_PORT1_CTRL1                          0x11
#define KSZ8863_PORT1_CTRL2                          0x12
#define KSZ8863_PORT1_CTRL3                          0x13
#define KSZ8863_PORT1_CTRL4                          0x14
#define KSZ8863_PORT1_CTRL5                          0x15
#define KSZ8863_PORT1_Q0_IG_LIMIT                    0x16
#define KSZ8863_PORT1_Q1_IG_LIMIT                    0x17
#define KSZ8863_PORT1_Q2_IG_LIMIT                    0x18
#define KSZ8863_PORT1_Q3_IG_LIMIT                    0x19
#define KSZ8863_PORT1_PHY_CTRL                       0x1A
#define KSZ8863_PORT1_LINKMD                         0x1B
#define KSZ8863_PORT1_CTRL12                         0x1C
#define KSZ8863_PORT1_CTRL13                         0x1D
#define KSZ8863_PORT1_STAT0                          0x1E
#define KSZ8863_PORT1_STAT1                          0x1F

#define KSZ8863_PORT2_CTRL0                          0x20
#define KSZ8863_PORT2_CTRL1                          0x21
#define KSZ8863_PORT2_CTRL2                          0x22
#define KSZ8863_PORT2_CTRL3                          0x23
#define KSZ8863_PORT2_CTRL4                          0x24
#define KSZ8863_PORT2_CTRL5                          0x25
#define KSZ8863_PORT2_Q0_IG_LIMIT                    0x26
#define KSZ8863_PORT2_Q1_IG_LIMIT                    0x27
#define KSZ8863_PORT2_Q2_IG_LIMIT                    0x28
#define KSZ8863_PORT2_Q3_IG_LIMIT                    0x29
#define KSZ8863_PORT2_PHY_CTRL                       0x2A
#define KSZ8863_PORT2_LINKMD                         0x2B
#define KSZ8863_PORT2_CTRL12                         0x2C
#define KSZ8863_PORT2_CTRL13                         0x2D
#define KSZ8863_PORT2_STAT0                          0x2E
#define KSZ8863_PORT2_STAT1                          0x2F

#define KSZ8863_PORT3_CTRL0                          0x30
#define KSZ8863_PORT3_CTRL1                          0x31
#define KSZ8863_PORT3_CTRL2                          0x32
#define KSZ8863_PORT3_CTRL3                          0x33
#define KSZ8863_PORT3_CTRL4                          0x34
#define KSZ8863_PORT3_CTRL5                          0x35
#define KSZ8863_PORT3_Q0_IG_LIMIT                    0x36
#define KSZ8863_PORT3_Q1_IG_LIMIT                    0x37
#define KSZ8863_PORT3_Q2_IG_LIMIT                    0x38
#define KSZ8863_PORT3_Q3_IG_LIMIT                    0x39
#define KSZ8863_PORT3_STAT1                          0x3F

#define KSZ8863_MAC_ADDR0                            0x70
#define KSZ8863_MAC_ADDR1                            0x71
#define KSZ8863_MAC_ADDR2                            0x72
#define KSZ8863_MAC_ADDR3                            0x73
#define KSZ8863_MAC_ADDR4                            0x74
#define KSZ8863_MAC_ADDR5                            0x75
#define KSZ8863_USER0                                0x76
#define KSZ8863_USER1                                0x77
#define KSZ8863_USER2                                0x78

#define KSZ8863_GLOBAL_CTRL1_TAIL_TAG_EN             BIT(6)
#define KSZ8863_GLOBAL_CTRL2_LEG_MAX_PKT_SIZ_CHK_ENA BIT(1)

#define KSZ8863_CTRL2_PORTn(n)                       (0x12 + ((n) * 0x10))
#define KSZ8863_CTRL2_TRANSMIT_EN                    BIT(2)
#define KSZ8863_CTRL2_RECEIVE_EN                     BIT(1)
#define KSZ8863_CTRL2_LEARNING_DIS                   BIT(0)

#define KSZ8863_STAT2_PORTn(n)                       (0x1E + ((n) * 0x10))
#define KSZ8863_STAT2_LINK_GOOD                      BIT(5)

#define KSZ8863_CHIP_ID0_ID_DEFAULT                  0x88
#define KSZ8863_CHIP_ID1_ID_DEFAULT                  0x31
#define KSZ8863_REGISTER_67                          0x43
#define KSZ8863_SOFTWARE_RESET_SET                   BIT(4)
#define KSZ8863_SOFTWARE_RESET_CLEAR                 0

enum {
	/* LAN ports for the ksz8863 switch */
	KSZ8863_PORT1 = 0,
	KSZ8863_PORT2,
	/* SWITCH <-> CPU port */
	KSZ8863_PORT3,
};

#define KSZ8863_REG_IND_CTRL_0                        0x79
#define KSZ8863_REG_IND_CTRL_1                        0x7A
#define KSZ8863_REG_IND_DATA_8                        0x7B
#define KSZ8863_REG_IND_DATA_7                        0x7C
#define KSZ8863_REG_IND_DATA_6                        0x7D
#define KSZ8863_REG_IND_DATA_5                        0x7E
#define KSZ8863_REG_IND_DATA_4                        0x7F
#define KSZ8863_REG_IND_DATA_3                        0x80
#define KSZ8863_REG_IND_DATA_2                        0x81
#define KSZ8863_REG_IND_DATA_1                        0x82
#define KSZ8863_REG_IND_DATA_0                        0x83

#define KSZ8863_STATIC_MAC_TABLE_VALID                BIT(3)
#define KSZ8863_STATIC_MAC_TABLE_OVRD                 BIT(4)
#define KSZ8863_STATIC_MAC_TABLE_USE_FID              BIT(5)

#define KSZ8XXX_CHIP_ID0                        KSZ8863_CHIP_ID0
#define KSZ8XXX_CHIP_ID1                        KSZ8863_CHIP_ID1
#define KSZ8XXX_CHIP_ID0_ID_DEFAULT             KSZ8863_CHIP_ID0_ID_DEFAULT
#define KSZ8XXX_CHIP_ID1_ID_DEFAULT             KSZ8863_CHIP_ID1_ID_DEFAULT
#define KSZ8XXX_FIRST_PORT                      KSZ8863_PORT1
#define KSZ8XXX_LAST_PORT                       KSZ8863_PORT3
#define KSZ8XXX_CPU_PORT                        KSZ8863_PORT3
#define KSZ8XXX_REG_IND_CTRL_0                  KSZ8863_REG_IND_CTRL_0
#define KSZ8XXX_REG_IND_CTRL_1                  KSZ8863_REG_IND_CTRL_1
#define KSZ8XXX_REG_IND_DATA_8                  KSZ8863_REG_IND_DATA_8
#define KSZ8XXX_REG_IND_DATA_7                  KSZ8863_REG_IND_DATA_7
#define KSZ8XXX_REG_IND_DATA_6                  KSZ8863_REG_IND_DATA_6
#define KSZ8XXX_REG_IND_DATA_5                  KSZ8863_REG_IND_DATA_5
#define KSZ8XXX_REG_IND_DATA_4                  KSZ8863_REG_IND_DATA_4
#define KSZ8XXX_REG_IND_DATA_3                  KSZ8863_REG_IND_DATA_3
#define KSZ8XXX_REG_IND_DATA_2                  KSZ8863_REG_IND_DATA_2
#define KSZ8XXX_REG_IND_DATA_1                  KSZ8863_REG_IND_DATA_1
#define KSZ8XXX_REG_IND_DATA_0                  KSZ8863_REG_IND_DATA_0
#define KSZ8XXX_STATIC_MAC_TABLE_VALID          KSZ8863_STATIC_MAC_TABLE_VALID
#define KSZ8XXX_STATIC_MAC_TABLE_OVRD           KSZ8863_STATIC_MAC_TABLE_OVRD
#define KSZ8XXX_STAT2_LINK_GOOD                 KSZ8863_STAT2_LINK_GOOD
#define KSZ8XXX_RESET_REG                       KSZ8863_REGISTER_67
#define KSZ8XXX_RESET_SET                       KSZ8863_SOFTWARE_RESET_SET
#define KSZ8XXX_RESET_CLEAR                     KSZ8863_SOFTWARE_RESET_CLEAR
#define KSZ8XXX_STAT2_PORTn                     KSZ8863_STAT2_PORTn
#define KSZ8XXX_SPI_CMD_RD                      KSZ8863_SPI_CMD_RD
#define KSZ8XXX_SPI_CMD_WR                      KSZ8863_SPI_CMD_WR
#define KSZ8XXX_SOFT_RESET_DURATION             1000
#define KSZ8XXX_HARD_RESET_WAIT                 10000

#endif /* __DSA_KSZ8863_H__ */
