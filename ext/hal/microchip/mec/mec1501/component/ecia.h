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

/** @file mec1501_ecia.h
 *MEC1501 EC Interrupt Aggregator Subsystem definitions
 */
/** @defgroup MEC1501 Peripherals ECIA
 */

#ifndef _ECIA_H
#define _ECIA_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

#define MCHP_ECIA_ADDR	0x4000E000ul
#define MCHP_FIRST_GIRQ	8u
#define MCHP_LAST_GIRQ	26u

#define MCHP_ECIA_GIRQ_NO_NVIC	 22u

#define MCHP_ECIA_AGGR_BITMAP ((1ul << 8) + (1ul << 9) + (1ul << 10) +\
	(1ul << 11) + (1ul << 12) + (1ul << 24) +\
	(1ul << 25) + (1ul << 26))

#define MCHP_ECIA_DIRECT_BITMAP	 ((1ul << 13) + (1ul << 14) +\
	(1ul << 15) + (1ul << 16) +\
	(1ul << 17) + (1ul << 18) +\
	(1ul << 19) + (1ul << 20) +\
	(1ul << 21) + (1ul << 23))

/*
 * ARM Cortex-M4 NVIC registers
 * External sources are grouped by 32-bit registers.
 * MEC15xx has 173 external sources requiring 6 32-bit registers.
  */
#define MCHP_NUM_NVIC_REGS	6u
#define MCHP_NVIC_SET_EN_BASE	0xE000E100ul
#define MCHP_NVIC_CLR_EN_BASE	0xE000E180ul
#define MCHP_NVIC_SET_PEND_BASE	0xE000E200ul
#define MCHP_NVIC_CLR_PEND_BASE	0xE000E280ul
#define MCHP_NVIC_ACTIVE_BASE	0xE000E800ul
#define MCHP_NVIC_PRI_BASE	0xE000E400ul

/* 0 <= n < MCHP_NUM_NVIC_REGS */
#define MCHP_NVIC_SET_EN(n) \
	REG32(MCHP_NVIC_SET_EN_BASE + ((uintptr_t)(n) << 2))

#define MCHP_NVIC_CLR_EN(n) \
	REG32(MCHP_NVIC_CLR_EN_BASE + ((uintptr_t)(n) << 2))

#define MCHP_NVIC_SET_PEND(n) \
	REG32(MCHP_NVIC_SET_PEND_BASE + ((uintptr_t)(n) << 2))

#define MCHP_NVIC_CLR_PEND(n) \
	REG32(MCHP_NVIC_CLR_PEND_BASE + ((uintptr_t)(n) << 2))

/*
 * ECIA registers
 * Implements 19 GIRQ's. GIRQ's aggregated interrupts source into one
 * set of registers.
 * For historical reason GIRQ's are numbered starting at 8 in the documentation.
 * This numbering only affects the ECIA BLOCK_EN_SET, BLOCK_EN_CLR, and
 * BLOCK_ACTIVE registers: GIRQ8 is bit[8], ..., GIRQ26 is bit[26].
 *
 * Each GIRQ is composed of 5 32-bit registers.
 * +00h = GIRQ08 Source containing RW/1C status bits
 * +04h = Enable Set write 1 to bit(s) to enable the corresponding source(s)
 * +08h = Read-Only Result = Source AND Enable-Set
 * +0Ch = Enable Clear write 1 to bit(s) to disable the corresponding source(s)
 * +14h = Reserved(unused).
 * +18h = GIRQ09 Source
 * ...
 * There are three other registers at offset 0x200, 0x204, and 0x208
 * 0x200: BLOCK_EN_SET bit == 1 allows bit-wise OR of all GIRQn source
 *	bits to be connected to NVIC GIRQn input.
 *	bit[8]=GIRQ8, bit[9]=GIRQ9, ..., bit[26]=GIRQ26
 * 0x204: BLOCK_EN_CLR bit == 1 disconnects bit-wise OR of GIRQn source
 *	bits from NVIC GIRQn input.
 * 0x208: BLOCK_ACTIVE (read-only)
 *	bit[8]=GIRQ8 has at least one source bit enabled and active.
 *	...
 *	bit[26]=GIRQ26 has at least one source bit enabled and active.
 *
 */

/* zero based logical numbering */
#define MCHP_GIRQ08_ZID		0u
#define MCHP_GIRQ09_ZID		1u
#define MCHP_GIRQ10_ZID		2u
#define MCHP_GIRQ11_ZID		3u
#define MCHP_GIRQ12_ZID		4u
#define MCHP_GIRQ13_ZID		5u
#define MCHP_GIRQ14_ZID		6u
#define MCHP_GIRQ15_ZID		7u
#define MCHP_GIRQ16_ZID		8u
#define MCHP_GIRQ17_ZID		9u
#define MCHP_GIRQ18_ZID		10u
#define MCHP_GIRQ19_ZID		11u
#define MCHP_GIRQ20_ZID		12u
#define MCHP_GIRQ21_ZID		13u
#define MCHP_GIRQ22_ZID		14u
#define MCHP_GIRQ23_ZID		15u
#define MCHP_GIRQ24_ZID		16u
#define MCHP_GIRQ25_ZID		17u
#define MCHP_GIRQ26_ZID		18u
#define MCHP_GIRQ_ZID_MAX	19u

#define MCHP_ECIA_BLK_ENSET_OFS		0x200ul
#define MCHP_ECIA_BLK_ENCLR_OFS		0x204ul
#define MCHP_ECIA_BLK_ACTIVE_OFS	0x208ul

#define MCHP_GIRQ_BLK_ENSET_ADDR \
	(MCHP_ECIA_ADDR + MCHP_ECIA_BLK_ENSET_OFS)

#define MCHP_GIRQ_BLK_ENCLR_ADDR \
	(MCHP_ECIA_ADDR + MCHP_ECIA_BLK_ENCLR_OFS)

#define MCHP_GIRQ_BLK_ACTIVE_ADDR \
	(MCHP_ECIA_ADDR + MCHP_ECIA_BLK_ACTIVE_OFS)

/* 8 <= n <= 26 */
#define MCHP_GIRQ_SRC_ADDR(n) \
	((MCHP_ECIA_ADDR + 0x00ul) + (((uint32_t)(n) - 8ul) * 0x14ul))

#define MCHP_GIRQ_ENSET_ADDR(n) \
	((MCHP_ECIA_ADDR + 0x04ul) + (((uint32_t)(n) - 8ul) * 0x14ul))

#define MCHP_GIRQ_RESULT_ADDR(n) \
	((MCHP_ECIA_ADDR + 0x08ul) + (((uint32_t)(n) - 8ul) * 0x14ul))

#define MCHP_GIRQ_ENCLR_ADDR(n) \
	((MCHP_ECIA_ADDR + 0x0Cul) + (((uint32_t)(n) - 8ul) * 0x14ul))

#define MCHP_GIRQ08_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(8)
#define MCHP_GIRQ08_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(8)
#define MCHP_GIRQ08_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(8)
#define MCHP_GIRQ08_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(8)

#define MCHP_GIRQ09_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(9)
#define MCHP_GIRQ09_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(9)
#define MCHP_GIRQ09_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(9)
#define MCHP_GIRQ09_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(9)

#define MCHP_GIRQ10_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(10)
#define MCHP_GIRQ10_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(10)
#define MCHP_GIRQ10_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(10)
#define MCHP_GIRQ10_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(10)

#define MCHP_GIRQ11_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(11)
#define MCHP_GIRQ11_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(11)
#define MCHP_GIRQ11_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(11)
#define MCHP_GIRQ11_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(11)

#define MCHP_GIRQ12_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(12)
#define MCHP_GIRQ12_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(12)
#define MCHP_GIRQ12_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(12)
#define MCHP_GIRQ12_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(12)

#define MCHP_GIRQ13_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(13)
#define MCHP_GIRQ13_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(13)
#define MCHP_GIRQ13_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(13)
#define MCHP_GIRQ13_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(13)

#define MCHP_GIRQ14_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(14)
#define MCHP_GIRQ14_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(14)
#define MCHP_GIRQ14_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(14)
#define MCHP_GIRQ14_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(14)

#define MCHP_GIRQ15_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(15)
#define MCHP_GIRQ15_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(15)
#define MCHP_GIRQ15_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(15)
#define MCHP_GIRQ15_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(15)

#define MCHP_GIRQ16_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(16)
#define MCHP_GIRQ16_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(16)
#define MCHP_GIRQ16_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(16)
#define MCHP_GIRQ16_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(16)

#define MCHP_GIRQ17_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(17)
#define MCHP_GIRQ17_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(17)
#define MCHP_GIRQ17_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(17)
#define MCHP_GIRQ17_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(17)

#define MCHP_GIRQ18_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(18)
#define MCHP_GIRQ18_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(18)
#define MCHP_GIRQ18_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(18)
#define MCHP_GIRQ18_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(18)

#define MCHP_GIRQ19_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(19)
#define MCHP_GIRQ19_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(19)
#define MCHP_GIRQ19_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(19)
#define MCHP_GIRQ19_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(19)

#define MCHP_GIRQ20_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(20)
#define MCHP_GIRQ20_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(20)
#define MCHP_GIRQ20_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(20)
#define MCHP_GIRQ20_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(20)

#define MCHP_GIRQ21_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(21)
#define MCHP_GIRQ21_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(21)
#define MCHP_GIRQ21_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(21)
#define MCHP_GIRQ21_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(21)

#define MCHP_GIRQ22_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(22)
#define MCHP_GIRQ22_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(22)
#define MCHP_GIRQ22_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(22)
#define MCHP_GIRQ22_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(22)

#define MCHP_GIRQ23_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(23)
#define MCHP_GIRQ23_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(23)
#define MCHP_GIRQ23_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(23)
#define MCHP_GIRQ23_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(23)

#define MCHP_GIRQ24_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(24)
#define MCHP_GIRQ24_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(24)
#define MCHP_GIRQ24_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(24)
#define MCHP_GIRQ24_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(24)

#define MCHP_GIRQ25_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(25)
#define MCHP_GIRQ25_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(25)
#define MCHP_GIRQ25_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(25)
#define MCHP_GIRQ25_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(25)

#define MCHP_GIRQ26_SRC_ADDR	MCHP_GIRQ_SRC_ADDR(26)
#define MCHP_GIRQ26_ENSET_ADDR	MCHP_GIRQ_ENSET_ADDR(26)
#define MCHP_GIRQ26_RESULT_ADDR	MCHP_GIRQ_RESULT_ADDR(26)
#define MCHP_GIRQ26_ENCLR_ADDR	MCHP_GIRQ_ENCLR_ADDR(26)

/*
 * Register access
 */
#define MCHP_GIRQ_BLK_ENSET() \
	REG32(MCHP_GIRQ_BLK_ENSET_ADDR)

#define MCHP_GIRQ_BLK_ENCLR() \
	REG32(MCHP_GIRQ_BLK_ENCLR_ADDR)

#define MCHP_GIRQ_BLK_ACTIVE() \
	REG32(MCHP_GIRQ_BLK_ACTIVE_ADDR)

/*
 * Set/clear GIRQ Block Enable
 * Check if block is active
 * 8 <= n <= 26 corresponding to GIRQ08, GIRQ09, ..., GIRQ26
 */
#define MCHP_GIRQ_BLK_SETEN(n) \
	REG32(MCHP_GIRQ_BLK_ENSET_ADDR) = (1ul << (uint32_t)(n))

#define MCHP_GIRQ_BLK_CLREN(n) \
	REG32(MCHP_GIRQ_BLK_ENCLR_ADDR) = (1ul << (uint32_t)(n))

#define MCHP_GIRQ_BLK_IS_ACTIVE(n) \
	((REG32(MCHP_GIRQ_BLK_ACTIVE_ADDR) & (1ul << (uint32_t)(n))) != 0ul)

/* 8 <= n <= 26 corresponding to GIRQ08, GIRQ09, ..., GIRQ26 */
#define MCHP_GIRQ_SRC(n) REG32(MCHP_GIRQ_SRC_ADDR(n))

#define MCHP_GIRQ_ENSET(n) REG32(MCHP_GIRQ_ENSET_ADDR(n))

#define MCHP_GIRQ_RESULT(n) REG32(MCHP_GIRQ_RESULT_ADDR(n))
#define MCHP_GIRQ_ENCLR(n)  REG32(MCHP_GIRQ_ENCLR_ADDR(n))

/*
 * 8 <= n <= 26 corresponding to GIRQ08, GIRQ09, ..., GIRQ26
 * 0 <= pos <= 31 the bit position of the peripheral interrupt source.
 */
#define MCHP_GIRQ_SRC_CLR(n, pos) \
	REG32(MCHP_GIRQ_SRC_ADDR(n)) = (1ul << (uint32_t)(pos))

#define MCHP_GIRQ_SET_EN(n, pos) \
	REG32(MCHP_GIRQ_ENSET_ADDR(n)) = (1ul << (uint32_t)(pos))

#define MCHP_GIRQ_CLR_EN(n, pos) \
	REG32(MCHP_GIRQ_ENCLR_ADDR(n)) = (1ul << (uint32_t)(pos))

#define MCHP_GIRQ_IS_RESULT(n, pos) \
	(REG32(MCHP_GIRQ_RESULT_ADDR(n)) & (1ul << (uint32_t)(pos)) != 0ul)

/* =========================================================================*/
/* ================	       ECIA			   ================ */
/* =========================================================================*/

enum MCHP_GIRQ_IDS {
	MCHP_GIRQ08_ID = 8,
	MCHP_GIRQ09_ID,
	MCHP_GIRQ10_ID,
	MCHP_GIRQ11_ID,
	MCHP_GIRQ12_ID,
	MCHP_GIRQ13_ID,
	MCHP_GIRQ14_ID,
	MCHP_GIRQ15_ID,
	MCHP_GIRQ16_ID,
	MCHP_GIRQ17_ID,
	MCHP_GIRQ18_ID,
	MCHP_GIRQ19_ID,
	MCHP_GIRQ20_ID,
	MCHP_GIRQ21_ID,
	MCHP_GIRQ22_ID,
	MCHP_GIRQ23_ID,
	MCHP_GIRQ24_ID,
	MCHP_GIRQ25_ID,
	MCHP_GIRQ26_ID,
	MCHP_GIRQ_ID_MAX,
};

/**
  * @brief EC Interrupt Aggregator (ECIA)
  */
#define MCHP_GIRQ_START_NUM	8u
#define MCHP_GIRQ_LAST_NUM	26u
#define MCHP_GIRQ_IDX(girq)	((uint32_t)(girq) - 8ul)
#define MCHP_GIRQ_IDX_FIRST	0u
#define MCHP_GIRQ_IDX_MAX	19u
#define MCHP_MAX_NVIC_IDX	6u

/* size = 0x14(20) bytes */
typedef struct girq_regs
{
	__IOM uint32_t SRC;
	__IOM uint32_t EN_SET;
	__IOM uint32_t RESULT;
	__IOM uint32_t EN_CLR;
	uint8_t RSVD1[4];
} GIRQ_Type;

typedef struct ecia_regs
{		/*!< (@ 0x4000E000) ECIA Structure   */
	GIRQ_Type GIRQ08;	/*!< (@ 0x0000) GIRQ08 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ09;	/*!< (@ 0x0014) GIRQ09 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ10;	/*!< (@ 0x0028) GIRQ10 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ11;	/*!< (@ 0x003C) GIRQ11 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ12;	/*!< (@ 0x0050) GIRQ12 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ13;	/*!< (@ 0x0064) GIRQ13 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ14;	/*!< (@ 0x0078) GIRQ14 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ15;	/*!< (@ 0x008C) GIRQ15 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ16;	/*!< (@ 0x00A0) GIRQ16 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ17;	/*!< (@ 0x00B4) GIRQ17 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ18;	/*!< (@ 0x00C8) GIRQ18 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ19;	/*!< (@ 0x00DC) GIRQ19 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ20;	/*!< (@ 0x00F0) GIRQ20 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ21;	/*!< (@ 0x0104) GIRQ21 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ22;	/*!< (@ 0x0118) GIRQ22 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ23;	/*!< (@ 0x012C) GIRQ23 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ24;	/*!< (@ 0x0140) GIRQ24 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ25;	/*!< (@ 0x0154) GIRQ25 Source, Enable Set, Result, Enable Clear, Reserved */
	GIRQ_Type GIRQ26;	/*!< (@ 0x0168) GIRQ26 Source, Enable Set, Result, Enable Clear, Reserved */
	uint8_t RSVD2[(0x0200ul - 0x017Cul)];	/* offsets 0x017C - 0x1FF */
	__IOM uint32_t BLK_EN_SET;	/*! (@ 0x00000200) Aggregated GIRQ output Enable Set */
	__IOM uint32_t BLK_EN_CLR;	/*! (@ 0x00000204) Aggregated GIRQ output Enable Clear */
	__IM uint32_t BLK_ACTIVE;	/*! (@ 0x00000204) GIRQ Active bitmap (RO) */
} ECIA_Type;

#endif				// #ifndef _ECIA_H
/* end ecia.h */
/**   @}
 */
