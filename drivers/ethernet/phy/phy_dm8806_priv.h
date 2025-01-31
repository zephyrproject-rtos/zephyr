/*
 * Copyright (c) 2024 Robert Slawinski <robert.slawinski1@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The absolute PHY address is 10 bit length. Last 5 bits for oldest
 * part of address - see Clause 22 of IEEE 802.3.
 */
#define DM8806_REGAD_WIDTH 0x5u

/* Mask for checksum error. Checksum status is present on the 8-th bit of the
 * Serial Bus Error Check Register (339h) [8]
 */
#define DM8806_SMI_ERR 8

/* PHY read opcode as part of the MDIO frame in Clause 22 */
#define DM8806_PHY_READ  0x2u
/* PHY write opcode as part of the MDIO frame in Clause 22 */
#define DM8806_PHY_WRITE 0x1u

/* Port 0~4 PHY Control Register. */
#define DM8806_PORTX_PHY_CONTROL_REGISTER 0x0u
/* 10 Mbit/s transfer with half duplex mask. */
#define DM8806_MODE_10_BASET_HALF_DUPLEX  0x0u
/* 10 Mbit/s transfer with full duplex mask. */
#define DM8806_MODE_10_BASET_FULL_DUPLEX  0x100u
/* 100 Mbit/s transfer with half duplex mask. */
#define DM8806_MODE_100_BASET_HALF_DUPLEX 0x2000u
/* 100 Mbit/s transfer with full duplex mask. */
#define DM8806_MODE_100_BASET_FULL_DUPLEX 0x2100u
/* Duplex mode ability offset. */
#define DM8806_DUPLEX_MODE                (1 << 8)
/* Power down mode offset. */
#define DM8806_POWER_DOWN                 (1 << 11)
/* Auto negotiation mode offset. */
#define DM8806_AUTO_NEGOTIATION           (1 << 12)
/* Link speed selection offset. */
#define DM8806_LINK_SPEED                 (1 << 13)

/* Port 0~4 Status Data Register. */
#define DM8806_PORTX_SWITCH_STATUS       0x10u
/* 10 Mbit/s transfer speed with half duplex. */
#define DM8806_SPEED_10MBPS_HALF_DUPLEX  0x00u
/* 10 Mbit/s transfer speed with full duplex. */
#define DM8806_SPEED_10MBPS_FULL_DUPLEX  0x01u
/* 100 Mbit/s transfer speed with half duplex. */
#define DM8806_SPEED_100MBPS_HALF_DUPLEX 0x02u
/* 100 Mbit/s transfer speed with full duplex. */
#define DM8806_SPEED_100MBPS_FULL_DUPLEX 0x03u
/* Speed and duplex mode status offset. */
#define DM8806_SPEED_AND_DUPLEX_OFFSET   0x01u
/* Speed and duplex mode staus mask. */
#define DM8806_SPEED_AND_DUPLEX_MASK     0x07u
/* Link status mask. */
#define DM8806_LINK_STATUS_MASK          0x1u

/* Switch Engine Registers */
/* Address Table Control And Status Register PHY Address */
#define DM8806_ADDR_TAB_CTRL_STAT_PHY_ADDR 0x15u
/* Address Table Control And Status Register Register SAddress */
#define DM8806_ADDR_TAB_CTRL_STAT_REG_ADDR 0x10u

/* Address Table Access bussy flag offset */
#define DM8806_ATB_S_OFFSET  0xf
/* Address Table Command Result flag offset */
#define DM8806_ATB_CR_OFFSET 0xd
/* Address Table Command Result flag mask */
#define DM8806_ATB_CR_MASK   0x3

/* Unicast Address Table Index*/
#define DM8806_UNICAST_ADDR_TAB   (1 << 0 | 1 << 1)
/* Multicast Address Table Index*/
#define DM8806_MULTICAST_ADDR_TAB (1 << 0)
/* IGMP Table Index*/
#define DM8806_IGMP_ADDR_TAB      (1 << 1)

/* Read a entry with sequence number of address table */
#define DM8806_ATB_CMD_READ   (1 << 2 | 1 << 3 | 1 << 4)
/* Write a entry with MAC address */
#define DM8806_ATB_CMD_WRITE  (1 << 2)
/* Delete a entry with MAC address */
#define DM8806_ATB_CMD_DELETE (1 << 3)
/* Search a entry with MAC address */
#define DM8806_ATB_CMD_SEARCH (1 << 2 | 1 << 3)
/* Clear one or more than one entries with Port or FID */
#define DM8806_ATB_CMD_CLEAR  (1 << 4)

/* Address Table Data 0 PHY Address */
#define DM8806_ADDR_TAB_DATA0_PHY_ADDR 0x15u
/* Address Table Data 0 Register Address */
#define DM8806_ADDR_TAB_DATA0_REG_ADDR 0x11u
/* Port number or port map mask*/
#define DM8806_ATB_PORT_MASK           0x1f

/* Address Table Data 1 PHY Address */
#define DM8806_ADDR_TAB_DATA1_PHY_ADDR 0x15u
/* Address Table Data 1 Register Address */
#define DM8806_ADDR_TAB_DATA1_REG_ADDR 0x12u

/* Address Table Data 2 PHY Address */
#define DM8806_ADDR_TAB_DATA2_PHY_ADDR 0x15u
/* Address Table Data 2 Register Address */
#define DM8806_ADDR_TAB_DATA2_REG_ADDR 0x13u

/* Address Table Data 3 PHY Address */
#define DM8806_ADDR_TAB_DATA3_PHY_ADDR 0x15u
/* Address Table Data 3 Register Address */
#define DM8806_ADDR_TAB_DATA3_REG_ADDR 0x14u

/* Address Table Data 4 PHY Address */
#define DM8806_ADDR_TAB_DATA4_PHY_ADDR 0x15u
/* Address Table Data 4 Register Address */
#define DM8806_ADDR_TAB_DATA4_REG_ADDR 0x15u

/* WoL Control Register PHY Address */
#define DM8806_WOLL_CTRL_REG_PHY_ADDR 0x15u
/* WoL Control Register Register Address */
#define DM8806_WOLL_CTRL_REG_REG_ADDR 0x1bu

/* Serial Bus Error Check PHY Address. */
#define DM8806_SMI_BUS_ERR_CHK_PHY_ADDRESS 0x19u
/* Serial Bus Error Check Register Address. */
#define DM8806_SMI_BUS_ERR_CHK_REG_ADDRESS 0x19u

/* Serial Bus Control PHY Address. */
#define DM8806_SMI_BUS_CTRL_PHY_ADDRESS 0x19u
/* Serial Bus Control Register Address. */
#define DM8806_SMI_BUS_CTRL_REG_ADDRESS 0x1au
/* SMI Bus Error Check Enable. */
#define DM8806_SMI_ECE                  BIT(0)

/* PHY address 0x18h */
#define DM8806_PHY_ADDRESS_18H 0x18u

/* Interrupt Status Register PHY Address. */
#define DM8806_INT_STAT_PHY_ADDR 0x18u
/* Interrupt Status Register Register Address. */
#define DM8806_INT_STAT_REG_ADDR 0x18u

/* Interrupt Mask & Control Register PHY Address. */
#define DM8806_INT_MASK_CTRL_PHY_ADDR 0x18u
/* Interrupt Mask & Control Register Register Address. */
#define DM8806_INT_MASK_CTRL_REG_ADDR 0x19u

/* Switch Register offset  */
#define DM8806_SWITCH_REGISTER_OFFSET 0x08

/* Energy Efficient Ethernet Control Register Address. */
#define DM8806_ENERGY_EFFICIENT_ETH_CTRL_REG_ADDR 0x1e

/* Energy Efficient Ethernet enable bit */
#define DM8806_EEE_EN BIT(15)

#define DM8806_PORT5_MAC_CONTROL     0x15u
/* Port 5 Force Speed control bit */
#define DM8806_P5_SPEED_100M         ~BIT(0)
/* Port 5 Force Duplex control bit */
#define DM8806_P5_FULL_DUPLEX        ~BIT(1)
/* Port 5 Force Link control bit. Only available in force mode. */
#define DM8806_P5_FORCE_LINK_ON      ~BIT(2)
/* Port 5 Force Mode Enable control bit. Only available for
 * MII/RevMII/RMII
 */
#define DM8806_P5_EN_FORCE           BIT(3)
/* Bit 4 is reserved and should not be use */
/* Port 5 50MHz Clock Output Enable control bit. Only available when Port 5
 * be configured as RMII
 */
#define DM8806_P5_50M_CLK_OUT_ENABLE BIT(5)
/* Port 5 Clock Source Selection control bit. Only available when Port 5
 * is configured as RMII
 */
#define DM8806_P5_50M_INT_CLK_SOURCE BIT(6)
/* Port 5 Output Pin Slew Rate. */
#define DM8806_P5_NORMAL_SLEW_RATE   ~BIT(7)
/* IRQ and LED Control Register. */
#define DM8806_IRQ_LED_CONTROL       0x17u
/* LED mode 0:
 * LNK_LED:
 * 100M link fail                       - LED off
 * 100M link ok and no TX/RX activity   - LED on
 * 100M link ok and TX/RX activity      - LED blinking
 * SPD_LED:
 * No colision:                         - LED off
 * Colision:                            - LED blinking
 * FDX_LED:
 * 10M link fail                        - LED off
 * 10M link ok and no TX/RX activity    - LED on
 * 10M link ok and TX/RX activity       - LED blinking
 */
#define DM8806_LED_MODE_0            ~(BIT(0) | BIT(1))
