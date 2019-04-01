/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file dma.h
 *MEC1501 DMA controller definitions
 */
/** @defgroup MEC1501 Peripherals DMA
 */

#ifndef _DMA_H
#define _DMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "regaccess.h"

#define MCHP_NUM_DMA_CHANNELS	12ul

#define MCHP_DMA_BLOCK_BASE_ADDR	0x40002400ul
#define MCHP_DMA_CHAN_OFFSET		0x40ul
#define MCHP_DMA_OFFSET_POF2		6u
#define MCHP_DMA_CHAN0_ADDR \
	((MCHP_DMA_BLOCK_BASE_ADDR) + (MCHP_DMA_CHAN_OFFSET))

#define MCHP_DMA_CHAN_ADDR(n) ((uintptr_t)(MCHP_DMA_CHAN0_ADDR) +\
	((uintptr_t)(n) << MCHP_DMA_OFFSET_POF2))

/*
 * DMA block PCR register and bit
 * Bit position applied to PCR Sleep Enable, Clock Req(RO), and Reset
 * registers.
 */
#define MCHP_DMA_PCR_SLP_EN_ADDR	0x40080134ul
#define MCHP_DMA_PCR_CLK_REQ_ADDR	0x40080154ul
#define MCHP_DMA_PCR_RST_ADDR		0x40080174ul
#define MCHP_DMA_PCR_SCR_POS		6u
#define MCHP_DMA_PCR_SCR_VAL		(1ul << 6u)

#define MCHP_DMA_GIRQ_ID		14u
#define MCHP_DMA_GIRQ_SRC_ADDR		0x4000E078ul
#define MCHP_DMA_GIRQ_EN_SET_ADDR	0x4000E07Cul
#define MCHP_DMA_GIRQ_RESULT_ADDR	0x4000E080ul
#define MCHP_DMA_GIRQ_EN_CLR_ADDR	0x4000E084ul

#define MCHP_DMA_GIRQ_NUM	14u
#define MCHP_DMA_GIRQ_IDX	((MCHP_DMA_GIRQ_NUM) - 8u)

/* Aggregated GIRQ14 NVIC input */
#define MCHP_DMA_AGGR_NVIC	6u

/*
 * Value used for GIRQ Source, Set Enable,
 * Result, and Clear Enable registers.
 * 0 <= ch < MCHP_NUM_DMA_CHANNELS
 */
#define MCHP_DMA_GIRQ_VAL(ch)	(1ul << (ch))

/*
 * DMA channel direct NVIC external interrupt inputs
 * 0 <= ch < 12
 */
#define MCHP_DMA_DIRECT_NVIC_NUM(ch)	(24ul + (ch))

/*
 * DMA channel direct NVIC external interrupt input from channel address.
 * Channels are located starting at offset 0x40 from DMA block base address.
 * Channels are spaced every 0x40 bytes.
 * DMA block has 1KB total register space.
 */
#define MCHP_DMA_DIRECT_NVIC_NUM_BA(chba) \
	(24ul + (((uintptr_t)(chba) - (uintptr_t)MCHP_DMA_CHAN_ADDR(0)) >> 6))

#define MCHP_DMA0_GIRQ_NVIC	(24u + 0u)
#define MCHP_DMA1_GIRQ_NVIC	(24u + 1u)
#define MCHP_DMA2_GIRQ_NVIC	(24u + 2u)
#define MCHP_DMA3_GIRQ_NVIC	(24u + 3u)
#define MCHP_DMA4_GIRQ_NVIC	(24u + 4u)
#define MCHP_DMA5_GIRQ_NVIC	(24u + 5u)
#define MCHP_DMA6_GIRQ_NVIC	(24u + 6u)
#define MCHP_DMA7_GIRQ_NVIC	(24u + 7u)
#define MCHP_DMA8_GIRQ_NVIC	(24u + 8u)
#define MCHP_DMA9_GIRQ_NVIC	(24u + 9u)
#define MCHP_DMA10_GIRQ_NVIC	(24u + 10u)
#define MCHP_DMA11_GIRQ_NVIC	(24u + 11u)

/*
 * GIRQ bit position from channel base address
 */
#define MCHP_DMA_GIRQ_POS_BA(chba) \
	(((uintptr_t)(chba) - (uintptr_t)MCHP_DMA_CHAN_ADDR(0)) >> 6)

#define MCHP_DMA_GIRQ_VAL_BA(chba) \
	(1ul << MCHP_DMA_GIRQ_POS_BA(chba))

#define MCHP_DMA0_GIRQ_POS	0u
#define MCHP_DMA1_GIRQ_POS	1u
#define MCHP_DMA2_GIRQ_POS	2u
#define MCHP_DMA3_GIRQ_POS	3u
#define MCHP_DMA4_GIRQ_POS	4u
#define MCHP_DMA5_GIRQ_POS	5u
#define MCHP_DMA6_GIRQ_POS	6u
#define MCHP_DMA7_GIRQ_POS	7u
#define MCHP_DMA8_GIRQ_POS	8u
#define MCHP_DMA9_GIRQ_POS	9u
#define MCHP_DMA10_GIRQ_POS	10u
#define MCHP_DMA11_GIRQ_POS	11u

#define MCHP_DMA0_GIRQ_VAL	(1ul << 0u)
#define MCHP_DMA1_GIRQ_VAL	(1ul << 1u)
#define MCHP_DMA2_GIRQ_VAL	(1ul << 2u)
#define MCHP_DMA3_GIRQ_VAL	(1ul << 3u)
#define MCHP_DMA4_GIRQ_VAL	(1ul << 4u)
#define MCHP_DMA5_GIRQ_VAL	(1ul << 5u)
#define MCHP_DMA6_GIRQ_VAL	(1ul << 6u)
#define MCHP_DMA7_GIRQ_VAL	(1ul << 7u)
#define MCHP_DMA8_GIRQ_VAL	(1ul << 8u)
#define MCHP_DMA9_GIRQ_VAL	(1ul << 9u)
#define MCHP_DMA10_GIRQ_VAL	(1ul << 10u)
#define MCHP_DMA11_GIRQ_VAL	(1ul << 11u)

/*
 * Device Numbers for Channel Control Reg Device Number Field
 */
#define MCHP_DMA_DEVNUM_SMB0_SLV	0ul
#define MCHP_DMA_DEVNUM_SMB0_MTR	1ul
#define MCHP_DMA_DEVNUM_SMB1_SLV	2ul
#define MCHP_DMA_DEVNUM_SMB1_MTR	3ul
#define MCHP_DMA_DEVNUM_SMB2_SLV	4ul
#define MCHP_DMA_DEVNUM_SMB2_MTR	5ul
#define MCHP_DMA_DEVNUM_SMB3_SLV	6ul
#define MCHP_DMA_DEVNUM_SMB3_MTR	7ul
#define MCHP_DMA_DEVNUM_SMB4_SLV	8ul
#define MCHP_DMA_DEVNUM_SMB4_RX		9ul
#define MCHP_DMA_DEVNUM_QMSPI_TX	10ul
#define MCHP_DMA_DEVNUM_QMSPI_RX	11ul
#define MCHP_DMA_DEVNUM_MAX		 12ul

#define MCHP_DMA_CHAN_REG_BLEN		0x40ul

/* DMA Block Layout
 * DMA Main registers start at block base
 * Three registers located at word boundaries (0, 4, 8)
 * Each channel starts at base + ((channel_number + 1) * DMA_CHAN_REG_BLEN)
 * DMA Main @ base
 * DMA Channel 0 @ base + DMA_CHAN_REG_BLEN
 * DMA Channel 1 @ base + (2 * DMA_CHAN_REG_BLEN)
 *
 */

/*
 * DMA Main Registers
 */
#define MCHP_DMAM_CTRL_OFS	0x00ul
#define MCHP_DMAM_PKT_RO_OFS	0x04ul
#define MCHP_DMAM_FSM_RO_OFS	0x08ul

#define MCHP_DMAM_CTRL_OFFSET		0x00ul
#define MCHP_DMAM_CTRL_MASK		0x03UL
#define MCHP_DMAM_CTRL_ENABLE		(1UL << 0)
#define MCHP_DMAM_CTRL_SOFT_RESET	(1UL << 1)

/*
 * DMA Main Register Access
 */
#define MCHP_DMAM_CTRL() \
	REG8_OFS(MCHP_DMA_BLOCK_BASE_ADDR, MCHP_DMAM_CTRL_OFS)

#define MCHP_DMAM_PKT_RO() \
	REG32_OFS(MCHP_DMA_BLOCK_BASE_ADDR, MCHP_DMAM_PKT_RO_OFS)

#define MCHP_DMAM_FSM_RO() \
	REG32_OFS(MCHP_DMA_BLOCK_BASE_ADDR, MCHP_DMAM_FSM_RO_OFS)

/*
 * DMA channel register offsets
 */
#define MCHP_DMA_ACTV_OFS	0x00ul
#define MCHP_DMA_MSTART_OFS	0x04ul
#define MCHP_DMA_MEND_OFS	0x08ul
#define MCHP_DMA_DSTART_OFS	0x0Cul
#define MCHP_DMA_CTRL_OFS	0x10ul
#define MCHP_DMA_ISTS_OFS	0x14ul
#define MCHP_DMA_IEN_OFS	0x18ul
#define MCHP_DMA_FSM_RO_OFS	0x1Cul
/* Channels 0 and 1 include optional ALU */
#define MCHP_DMA_ALU_CTRL_OFS	0x20ul
#define MCHP_DMA_ALU_DATA_OFS	0x24ul
#define MCHP_DMA_ALU_STS_RO_OFS	0x28ul

/*
 * DMA Channel register addresses
 */
#define MCHP_DMA_ACTV_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_ACTV_OFS))

#define MCHP_DMA_MSTART_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_MSTART_OFS))

#define MCHP_DMA_MEND_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_MEND_OFS))

#define MCHP_DMA_DSTART_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_DSTART_OFS))

#define MCHP_DMA_CTRL_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_CTRL_OFS))

#define MCHP_DMA_ISTS_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_ISTS_OFS))

#define MCHP_DMA_IEND_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_IEN_OFS))

#define MCHP_DMA_FSM_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_FSM_RO_OFS))

/* Channels 0 and 1 include optional ALU */
#define MCHP_DMA_ALU_CTRL_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_ALU_CTRL_OFS))

#define MCHP_DMA_ALU_DATA_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_ALU_DATA_OFS))

#define MCHP_DMA_ALU_STS_RO_ADDR(ch) \
	(uintptr_t)(MCHP_DMA_CHAN_ADDR(ch) + (MCHP_DMA_ALU_STS_RO_OFS))

/*
 * DMA Channel Register Access
 * ch = channel ID: 0 <= ch < MCHP_NUM_DMA_CHANNELS
 */
#define MCHP_DMA_ACTV(ch) \
	REG8_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_ACTV_OFS)

#define MCHP_DMA_MSTART(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_MSTART_OFS)

#define MCHP_DMA_MEND(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_MEND_OFS)

#define MCHP_DMA_DSTART(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_DSTART_OFS)

#define MCHP_DMA_CTRL(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_CTRL_OFS)

#define MCHP_DMA_ISTS(ch) \
	REG8_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_ISTS_OFS)

#define MCHP_DMA_IEN(ch) \
	REG8_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_IEN_OFS)

#define MCHP_DMA_FSM(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_FSM_RO_OFS)

#define MCHP_DMA_ALU_EN(ch) \
	REG8_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_ALU_CTRL_OFS)

#define MCHP_DMA_ALU_DATA(ch) \
	REG32_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_ALU_DATA_OFS)

#define MCHP_DMA_ALU_STS_RO(ch) \
	REG8_OFS(MCHP_DMA_CHAN_ADDR(ch), MCHP_DMA_ALU_STS_RO_OFS)

/*
 * DMA Channel Register Access by base address
 * chba = channel base address (start of channel registers)
 */
#define MCHP_DMA_ACTV_BA(chba) \
	REG8_OFS(chba, MCHP_DMA_ACTV_OFS)

#define MCHP_DMA_MSTART_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_MSTART_OFS)

#define MCHP_DMA_MEND_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_MEND_OFS)

#define MCHP_DMA_DSTART_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_DSTART_OFS)

#define MCHP_DMA_CTRL_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_CTRL_OFS)

#define MCHP_DMA_ISTS_BA(chba) \
	REG8_OFS(chba, MCHP_DMA_ISTS_OFS)

#define MCHP_DMA_IEN_BA(chba) \
	REG8_OFS(chba, MCHP_DMA_IEN_OFS)

#define MCHP_DMA_FSM_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_FSM_RO_OFS)

#define MCHP_DMA_ALU_EN_BA(chba) \
	REG8_OFS(chba, MCHP_DMA_ALU_CTRL_OFS)

#define MCHP_DMA_ALU_DATA_BA(chba) \
	REG32_OFS(chba, MCHP_DMA_ALU_DATA_OFS)

#define MCHP_DMA_ALU_STS_RO_BA(chba) \
	REG8_OFS(chba, MCHP_DMA_ALU_STS_RO_OFS)

/*
 * Channel Activate, Offset 0x00, R/W
 */
#define MCHP_DMA_ACTV_REG_MASK	0x01ul
#define MCHP_DMA_ACTV_VAL	(1UL << 0)

/*
 * Target (destination) Start Memory Address, Offset 0x04
 */
#define MCHP_DMA_MSTART_REG_MASK	0xFFFFFFFFul

/*
 * Target (destination) End Memory Address, Offset 0x08
 */
#define MCHP_DMA_MEND_REG_MASK		0xFFFFFFFFul

/*
 * Source (device) Address, Offset 0x0C
 */
#define MCHP_DMA_DSTART_REG_MASK	0xFFFFFFFFul

/*
 * Control, Offset 0x10
 */
#define MCHP_DMA_C_REG_MASK		0x037FFF3Ful
#define MCHP_DMA_C_RUN			(1UL << 0)
#define MCHP_DMA_C_REQ_STS_RO		(1UL << 1)
#define MCHP_DMA_C_DONE_STS_RO		(1UL << 2)
#define MCHP_DMA_C_CHAN_STS_MASK	(0x03UL << 3)
#define MCHP_DMA_C_BUSY_STS_POS		5u
#define MCHP_DMA_C_BUSY_STS		(1UL << 5)
#define MCHP_DMA_C_DIR_POS		8u
#define MCHP_DMA_C_DEV2MEM		(0UL << 8)
#define MCHP_DMA_C_MEM2DEV		(1UL << 8)
#define MCHP_DMA_C_DEV_NUM_POS		9u
#define MCHP_DMA_C_DEV_NUM_MASK0	0x7Ful
#define MCHP_DMA_C_DEV_NUM_MASK		(0x7F << 9)
#define MCHP_DMA_C_NO_INCR_MEM		(0UL << 16)
#define MCHP_DMA_C_INCR_MEM		(1UL << 16)
#define MCHP_DMA_C_NO_INCR_DEV		(0UL << 17)
#define MCHP_DMA_C_INCR_DEV		(1UL << 17)
#define MCHP_DMA_C_LOCK_CHAN		(1UL << 18)
#define MCHP_DMA_C_DIS_HWFLC		(1UL << 19)
#define MCHP_DMA_C_XFRU_POS		20u
#define MCHP_DMA_C_XFRU_MASK0		0x07ul
#define MCHP_DMA_C_XFRU_MASK		(0x07ul << 20)
#define MCHP_DMA_C_XFRU_1B		(1UL << MCHP_DMA_C_XFRU_POS)
#define MCHP_DMA_C_XFRU_2B		(2UL << MCHP_DMA_C_XFRU_POS)
#define MCHP_DMA_C_XFRU_4B		(4UL << MCHP_DMA_C_XFRU_POS)
#define MCHP_DMA_C_XFER_GO		(1UL << 24)
#define MCHP_DMA_C_XFER_ABORT		(1UL << 25)
/* combine direction and device number fields */
#define MCHP_DMA_C_DEVDIR_POS		8u
#define MCHP_DMA_C_DEVDIR_MASK0		0xFFul
#define MCHP_DMA_C_DEVDIR_MASK		(0xFFul << 8)

/*
 * Channel Interrupt Status, Offset 0x14
 */
#define MCHP_DMA_STS_REG_MASK		0x07ul
#define MCHP_DMA_STS_BUS_ERR		(1UL << 0)
#define MCHP_DMA_STS_FLOW_CTRL_ERR	(1UL << 1)
#define MCHP_DMA_STS_DONE		(1UL << 2)
#define MCHP_DMA_STS_ALL		0x07ul

/*
 * Channel Interrupt Enable, Offset 0x18
 */
#define MCHP_DMA_IEN_REG_MASK		0x07ul
#define MCHP_DMA_IEN_BUS_ERR		(1UL << 0)
#define MCHP_DMA_IEN_FLOW_CTRL_ERR	(1UL << 1)
#define MCHP_DMA_IEN_DONE		(1UL << 2)
#define MCHP_DMA_IEN_ALL		0x07ul

/*
 * Channel FSM (read-only), Offset 0x1C
 */
#define MCHP_DMA_FSM_REG_MASK		0x0FFFFul

/*
 * DMA Block with optional ALU includes four extra registers.
 * Channel's total register allocation is 0x40
 */

/*
 * ALU Control, Offset 0x20
 */
#define MCHP_DMA_ALU_CTRL_MASK		0x03ul
#define MCHP_DMA_ALU_ENABLE_POS		0
#define MCHP_DMA_ALU_MASK		(1ul << (MCHP_DMA_ALU_ENABLE_POS))
#define MCHP_DMA_ALU_DISABLE		(0ul << (MCHP_DMA_ALU_ENABLE_POS))
#define MCHP_DMA_ALU_ENABLE		(1ul << (MCHP_DMA_ALU_ENABLE_POS))
#define MCHP_DMA_ALU_POST_XFER_EN_POS	1u
#define MCHP_DMA_ALU_POST_XFER_EN_MASK	(1ul << (MCHP_DMA_ALU_POST_XFER_EN_POS))
#define MCHP_DMA_ALU_POST_XFER_DIS	(0ul << (MCHP_DMA_ALU_POST_XFER_EN_POS))
#define MCHP_DMA_ALU_POST_XFER_EN	(1ul << (MCHP_DMA_ALU_POST_XFER_EN_POS))

/*
 * ALU Data, Offset 0x24
 */
#define MCHP_DMA_ALU_DATA_MASK		0xFFFFFFFFul

/*
 * ALU Status, Offset 0x28 Read-Only
 */
#define MCHP_DMA_ALU_STS_MASK		0x0Ful
#define MCHP_DMA_ALU_STS_DONE_POS	0u
#define MCHP_DMA_ALU_STS_RUN_POS	1u
#define MCHP_DMA_ALU_STS_XFR_DONE_POS	2u
#define MCHP_DMA_ALU_STS_DATA_RDY_POS	3u
#define MCHP_DMA_ALU_STS_DONE		(1ul << (MCHP_DMA_ALU_STS_DONE_POS))
#define MCHP_DMA_ALU_STS_RUN		(1ul << (MCHP_DMA_ALU_STS_RUN_POS))
#define MCHP_DMA_ALU_STS_XFR_DONE	(1ul << (MCHP_DMA_ALU_STS_XFR_DONE_POS))
#define MCHP_DMA_ALU_STS_DATA_RDY	(1ul << ((MCHP_DMA_ALU_STS_DATA_RDY_POS))

/*
 * ALU Test, Offset 0x2C Reserved
 */
#define MCHP_DMA_ALU_TEST_MASK		0xFFFFFFFFul

/*
 * Channel 0 has ALU for CRC32
 * Channel 1 has ALU for memory fill
 * Channels 2-11 do not implement an ALU
 */
#define MCHP_DMA_NUM_CHAN		12u
#define MCHP_DMA_CHAN_SPACING		0x40ul
#define MCHP_DMA_CHAN_SPACING_POF2	6u

#define MCHP_DMA_WCRC_CHAN_ID		0u
#define MCHP_DMA_WMF_CHAN_ID		1u
#define MCHP_MAX_DMA_CHAN		12u
#define MCHP_NUM_DMA_CHAN_NO_ALU	((MCHP_MAX_DMA_CHAN) - 2)

/**
  * @brief DMA Main (DMAM)
  */
typedef struct dma_main_regs
{		/*!< (@ 0x40002400) DMA Structure */
	__IOM uint8_t ACTRST;	/*!< (@ 0x00000000) DMA block activate/reset */
	uint8_t RSVDA[3];
	__IM uint32_t DATA_PKT;	/*!< (@ 0x00000004) DMA data packet (RO) */
	__IM uint32_t ARB_FSM;	/*!< (@ 0x00000008) DMA Arbiter FSM (RO) */
} DMAM_Type;

/*
 * NOTE: structure size is 0x40 (64) bytes as each channel
 * is spaced every 0x40 bytes from DMA block base address.
 * Channel 0 starts at offset 0x40 from DMA Main base address.
 * Channels 0 and 1 include an ALU for special operations on data
 * they transfer.
 * Channel 0 ALU is specialized for CRC-32 calculations.
 * Channel 1 ALU is specialized for memory fill.
 */

/**
  * @brief DMA Channels 0 and 1 with ALU
  */
typedef struct dma_chan_alu_regs
{
	__IOM uint8_t ACTV;	/*!< (@ 0x00000000) DMA channel activate  */
	uint8_t RSVD1[3];
	__IOM uint32_t MSTART;	/*!< (@ 0x00000004) DMA channel memory start address  */
	__IOM uint32_t MEND;	/*!< (@ 0x00000008) DMA channel memory end address	*/
	__IOM uint32_t DSTART;	/*!< (@ 0x0000000C) DMA channel device start address  */
	__IOM uint32_t CTRL;	/*!< (@ 0x00000010) DMA channel control	 */
	__IOM uint8_t ISTS;	/*!< (@ 0x00000014) DMA channel interrupt status  */
	uint8_t RSVD2[3];
	__IOM uint8_t IEN;	/*!< (@ 0x00000018) DMA channel interrupt enable  */
	uint8_t RSVD3[3];
	__IM uint32_t FSM;	/*!< (@ 0x0000001C) DMA channel FSM (RO)  */
	__IOM uint8_t ALU_EN;	/*!< (@ 0x00000020) DMA channels [0-1] ALU Enable */
	uint8_t RSVD4[3];
	__IOM uint32_t ALU_DATA;	/*!< (@ 0x00000024) DMA channels [0-1] ALU Data */
	__IOM uint8_t ALU_STS;	/*!< (@ 0x00000028) DMA channels [0-1] ALU post status (RO) */
	uint8_t RSVD5[3];
	__IM uint32_t ALU_FSM;	/*!< (@ 0x0000002C) DMA channels [0-1] ALU FSM (RO) */
	uint8_t RSVD6[16];	/* pad to 0x40(64) byte size */
} DMA_CHAN_ALU_Type;

/**
  * @brief DMA Channels 2 through 11 no ALU
  */
typedef struct dma_chan_regs
{
	__IOM uint8_t ACTV;	/*!< (@ 0x00000000) DMA channel activate  */
	uint8_t RSVD1[3];
	__IOM uint32_t MSTART;	/*!< (@ 0x00000004) DMA channel memory start address  */
	__IOM uint32_t MEND;	/*!< (@ 0x00000008) DMA channel memory end address	*/
	__IOM uint32_t DSTART;	/*!< (@ 0x0000000C) DMA channel device start address  */
	__IOM uint32_t CTRL;	/*!< (@ 0x00000010) DMA channel control	 */
	__IOM uint8_t ISTS;	/*!< (@ 0x00000014) DMA channel interrupt status  */
	uint8_t RSVD2[3];
	__IOM uint8_t IEN;	/*!< (@ 0x00000018) DMA channel interrupt enable  */
	uint8_t RSVD3[3];
	__IM uint32_t FSM;	/*!< (@ 0x0000001C) DMA channel FSM (RO)  */
	uint8_t RSVD4[0x20];	/* pad to 0x40(64) byte size */
} DMA_CHAN_Type;

#endif				// #ifndef _DMA_H
/* end dma.h */
/**	  @}
 */
