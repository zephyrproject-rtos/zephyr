/* SPDX-License-Identifier: Apache-2.0 */
/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
 * Copyright (c) 2018-2019 Linaro Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This header is originally based on mbedOS header
 * targets/TARGET_ARM_SSG/TARGET_CM3DS_MPS2/device/drivers/smsc9220_eth.h,
 * but was considerably refactored since then.
 */

/* This file is the re-implementation of mps2_ethernet_api and Selftest's
 * ETH_MPS2.
 * MPS2 Selftest:https://silver.arm.com/browse/VEI10 ->
 *     \ISCM-1-0\AN491\software\Selftest\v2m_mps2\
 */
#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_SMSC911X_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_SMSC911X_PRIV_H_

#ifndef __I
#define __I
#endif
#ifndef __O
#define __O
#endif
#ifndef __IO
#define __IO
#endif

#define uint32_t uint32_t
#define uint16_t uint16_t
#define uint8_t uint8_t

#define GET_BITFIELD(val, lsb, msb) \
	(((val) >> (lsb)) & ((1 << ((msb) - (lsb) + 1)) - 1))
#define BFIELD(val, name) GET_BITFIELD(val, name ## _Lsb, name ## _Msb)
#define SMSC9220_BFIELD(reg, bfield) BFIELD(SMSC9220->reg, reg ## _ ## bfield)

/******************************************************************************/
/*                       SMSC9220 Register Definitions                        */
/******************************************************************************/

typedef struct {
/*   Receive FIFO Ports (offset 0x0) */
__I	uint32_t  RX_DATA_PORT;
	uint32_t  RESERVED1[0x7];
/*   Transmit FIFO Ports (offset 0x20) */
__O	uint32_t  TX_DATA_PORT;
	uint32_t  RESERVED2[0x7];

/*   Receive FIFO status port (offset 0x40) */
__I	uint32_t  RX_STAT_PORT;
/*   Receive FIFO status peek (offset 0x44) */
__I	uint32_t  RX_STAT_PEEK;
/*   Transmit FIFO status port (offset 0x48) */
__I	uint32_t  TX_STAT_PORT;
/*   Transmit FIFO status peek (offset 0x4C) */
__I	uint32_t  TX_STAT_PEEK;

/*   Chip ID and Revision (offset 0x50) */
__I	uint32_t  ID_REV;
/*   Main Interrupt Configuration (offset 0x54) */
__IO	uint32_t  IRQ_CFG;
/*   Interrupt Status (offset 0x58) */
__IO	uint32_t  INT_STS;
/*   Interrupt Enable Register (offset 0x5C) */
__IO	uint32_t  INT_EN;
/*   Reserved for future use (offset 0x60) */
	uint32_t  RESERVED3;
/*   Read-only byte order testing register 87654321h (offset 0x64) */
__I	uint32_t  BYTE_TEST;
/*   FIFO Level Interrupts (offset 0x68) */
__IO	uint32_t  FIFO_INT;
/*   Receive Configuration (offset 0x6C) */
__IO	uint32_t  RX_CFG;
/*   Transmit Configuration (offset 0x70) */
__IO	uint32_t  TX_CFG;
/*   Hardware Configuration (offset 0x74) */
__IO	uint32_t  HW_CFG;
/*   RX Datapath Control (offset 0x78) */
__IO	uint32_t  RX_DP_CTRL;
/*   Receive FIFO Information (offset 0x7C) */
__I	uint32_t  RX_FIFO_INF;
/*   Transmit FIFO Information (offset 0x80) */
__I	uint32_t  TX_FIFO_INF;
/*   Power Management Control (offset 0x84) */
__IO	uint32_t  PMT_CTRL;
/*   General Purpose IO Configuration (offset 0x88) */
__IO	uint32_t  GPIO_CFG;
/*   General Purpose Timer Configuration (offset 0x8C) */
__IO	uint32_t  GPT_CFG;
/*   General Purpose Timer Count (offset 0x90) */
__I	uint32_t  GPT_CNT;
/*   Reserved for future use (offset 0x94) */
	uint32_t  RESERVED4;
/*   WORD SWAP Register (offset 0x98) */
__IO	uint32_t  ENDIAN;
/*   Free Run Counter (offset 0x9C) */
__I	uint32_t  FREE_RUN;
/*   RX Dropped Frames Counter (offset 0xA0) */
__I	uint32_t  RX_DROP;
/*   MAC CSR Synchronizer Command (offset 0xA4) */
__IO	uint32_t  MAC_CSR_CMD;
/*   MAC CSR Synchronizer Data (offset 0xA8) */
__IO	uint32_t  MAC_CSR_DATA;
/*   Automatic Flow Control Configuration (offset 0xAC) */
__IO	uint32_t  AFC_CFG;
/*   EEPROM Command (offset 0xB0) */
__IO	uint32_t  E2P_CMD;
/*   EEPROM Data (offset 0xB4) */
__IO	uint32_t  E2P_DATA;

} SMSC9220_TypeDef;

#define HW_CFG_SRST BIT(0)

#define RX_STAT_PORT_PKT_LEN_Lsb 16
#define RX_STAT_PORT_PKT_LEN_Msb 29

#define PMT_CTRL_READY BIT(0)

#define RX_DP_CTRL_RX_FFWD BIT(31)

#define RX_FIFO_INF_RXSUSED_Lsb 16
#define RX_FIFO_INF_RXSUSED_Msb 23
#define RX_FIFO_INF_RXDUSED_Lsb 0
#define RX_FIFO_INF_RXDUSED_Msb 15

#define MAC_CSR_CMD_BUSY  BIT(31)
#define MAC_CSR_CMD_READ  BIT(30)
#define MAC_CSR_CMD_WRITE 0

/* SMSC9220 MAC Registers       Indices */
#define SMSC9220_MAC_CR         0x1
#define SMSC9220_MAC_ADDRH      0x2
#define SMSC9220_MAC_ADDRL      0x3
#define SMSC9220_MAC_HASHH      0x4
#define SMSC9220_MAC_HASHL      0x5
#define SMSC9220_MAC_MII_ACC    0x6
#define SMSC9220_MAC_MII_DATA   0x7
#define SMSC9220_MAC_FLOW       0x8
#define SMSC9220_MAC_VLAN1      0x9
#define SMSC9220_MAC_VLAN2      0xA
#define SMSC9220_MAC_WUFF       0xB
#define SMSC9220_MAC_WUCSR      0xC

#define MAC_MII_ACC_MIIBZY BIT(0)
#define MAC_MII_ACC_WRITE  BIT(1)
#define MAC_MII_ACC_READ   0

/* SMSC9220 PHY Registers       Indices */
#define SMSC9220_PHY_BCONTROL   0
#define SMSC9220_PHY_BSTATUS    1
#define SMSC9220_PHY_ID1        2
#define SMSC9220_PHY_ID2        3
#define SMSC9220_PHY_ANEG_ADV   4
#define SMSC9220_PHY_ANEG_LPA   5
#define SMSC9220_PHY_ANEG_EXP   6
#define SMSC9220_PHY_MCONTROL   17
#define SMSC9220_PHY_MSTATUS    18
#define SMSC9220_PHY_CSINDICATE 27
#define SMSC9220_PHY_INTSRC     29
#define SMSC9220_PHY_INTMASK    30
#define SMSC9220_PHY_CS         31

#ifndef SMSC9220_BASE
#define SMSC9220_BASE           DT_INST_REG_ADDR(0)
#endif

#define SMSC9220                ((volatile SMSC9220_TypeDef *)SMSC9220_BASE)

enum smsc9220_interrupt_source {
	SMSC9220_INTERRUPT_GPIO0 = 0,
	SMSC9220_INTERRUPT_GPIO1 = 1,
	SMSC9220_INTERRUPT_GPIO2 = 2,
	SMSC9220_INTERRUPT_RXSTATUS_FIFO_LEVEL = 3,
	SMSC9220_INTERRUPT_RXSTATUS_FIFO_FULL = 4,
	/* 5 Reserved according to Datasheet */
	SMSC9220_INTERRUPT_RX_DROPPED_FRAME = 6,
	SMSC9220_INTERRUPT_TXSTATUS_FIFO_LEVEL = 7,
	SMSC9220_INTERRUPT_TXSTATUS_FIFO_FULL = 8,
	SMSC9220_INTERRUPT_TXDATA_FIFO_AVAILABLE = 9,
	SMSC9220_INTERRUPT_TXDATA_FIFO_OVERRUN = 10,
	/* 11, 12 Reserved according to Datasheet */
	SMSC9220_INTERRUPT_TRANSMIT_ERROR = 13,
	SMSC9220_INTERRUPT_RECEIVE_ERROR = 14,
	SMSC9220_INTERRUPT_RECEIVE_WATCHDOG_TIMEOUT = 15,
	SMSC9220_INTERRUPT_TXSTATUS_OVERFLOW = 16,
	SMSC9220_INTERRUPT_POWER_MANAGEMENT = 17,
	SMSC9220_INTERRUPT_PHY = 18,
	SMSC9220_INTERRUPT_GP_TIMER = 19,
	SMSC9220_INTERRUPT_RX_DMA = 20,
	SMSC9220_INTERRUPT_TX_IOC = 21,
	/* 22 Reserved according to Datasheet*/
	SMSC9220_INTERRUPT_RX_DROPPED_FRAME_HALF = 23,
	SMSC9220_INTERRUPT_RX_STOPPED = 24,
	SMSC9220_INTERRUPT_TX_STOPPED = 25,
	/* 26 - 30 Reserved according to Datasheet*/
	SMSC9220_INTERRUPT_SW = 31
};

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_SMSC911X_PRIV_H_ */
