/*
 * Copyright (c) 2024 Robert Slawinski <robert.slawinski1@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Port 0~4 PHY Control Register. */
#define PORTX_PHY_CONTROL_REGISTER 0x0u
/* 10 Mbit/s transfer with half duplex mask. */
#define MODE_10_BASET_HALF_DUPLEX  0x0u
/* 10 Mbit/s transfer with full duplex mask. */
#define MODE_10_BASET_FULL_DUPLEX  0x100u
/* 100 Mbit/s transfer with half duplex mask. */
#define MODE_100_BASET_HALF_DUPLEX 0x2000u
/* 100 Mbit/s transfer with full duplex mask. */
#define MODE_100_BASET_FULL_DUPLEX 0x2100u
/* Duplex mode ability offset. */
#define DUPLEX_MODE                (1 << 8)
/* Power down mode offset. */
#define POWER_DOWN                 (1 << 11)
/* Auto negotiation mode offset. */
#define AUTO_NEGOTIATION           (1 << 12)
/* Link speed selection offset. */
#define LINK_SPEED                 (1 << 13)

/* Port 0~4 Status Data Register. */
#define PORTX_SWITCH_STATUS       0x10u
/* 10 Mbit/s transfer speed with half duplex. */
#define SPEED_10MBPS_HALF_DUPLEX  0x00u
/* 10 Mbit/s transfer speed with full duplex. */
#define SPEED_10MBPS_FULL_DUPLEX  0x01u
/* 100 Mbit/s transfer speed with half duplex. */
#define SPEED_100MBPS_HALF_DUPLEX 0x02u
/* 100 Mbit/s transfer speed with full duplex. */
#define SPEED_100MBPS_FULL_DUPLEX 0x03u
/* Speed and duplex mode status offset. */
#define SPEED_AND_DUPLEX_OFFSET   0x01u
/* Speed and duplex mode staus mask. */
#define SPEED_AND_DUPLEX_MASK     0x07u
/* Link status mask. */
#define LINK_STATUS_MASK          0x1u

/* PHY address 0x18h */
#define PHY_ADDRESS_18H 0x18u

#define PORT5_MAC_CONTROL     0x15u
/* Port 5 Force Speed control bit */
#define P5_SPEED_100M         ~BIT(0)
/* Port 5 Force Duplex control bit */
#define P5_FULL_DUPLEX        ~BIT(1)
/* Port 5 Force Link control bit. Only available in force mode. */
#define P5_FORCE_LINK_ON      ~BIT(2)
/* Port 5 Force Mode Enable control bit. Only available for
 * MII/RevMII/RMII
 */
#define P5_EN_FORCE           BIT(3)
/* Bit 4 is reserved and should not be use */
/* Port 5 50MHz Clock Output Enable control bit. Only available when Port 5
 * be configured as RMII
 */
#define P5_50M_CLK_OUT_ENABLE BIT(5)
/* Port 5 Clock Source Selection control bit. Only available when Port 5
 * is configured as RMII
 */
#define P5_50M_INT_CLK_SOURCE BIT(6)
/* Port 5 Output Pin Slew Rate. */
#define P5_NORMAL_SLEW_RATE   ~BIT(7)
/* IRQ and LED Control Register. */
#define IRQ_LED_CONTROL       0x17u
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
#define LED_MODE_0            ~(BIT(0) | BIT(1))
