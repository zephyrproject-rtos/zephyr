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

/** @file vbat.h
 *MEC1501 VBAT Registers and memory definitions
 */
/** @defgroup MEC1501 Peripherals VBAT
 */

#ifndef _VBAT_H
#define _VBAT_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/*
 * VBAT Registers Registers
 */

#define MCHP_VBAT_REGISTERS_ADDR	0x4000A400ul
#define MCHP_VBAT_MEMORY_ADDR		0x4000A800ul
#define MCHP_VBAT_MEMORY_SIZE		64ul

/*
 * Offset 0x00 Power-Fail and Reset Status
 */
#define MCHP_VBATR_PFRS_OFS		0ul
#define MCHP_VBATR_PFRS_MASK		0x7Cul
#define MCHP_VBATR_PFRS_SYS_RST_POS	2u
#define MCHP_VBATR_PFRS_JTAG_POS	3u
#define MCHP_VBATR_PFRS_RESETI_POS	4u
#define MCHP_VBATR_PFRS_WDT_POS		5u
#define MCHP_VBATR_PFRS_SYSRESETREQ_POS	6u
#define MCHP_VBATR_PFRS_VBAT_RST_POS	7u

#define MCHP_VBATR_PFRS_SYS_RST		(1ul << 2)
#define MCHP_VBATR_PFRS_JTAG		(1ul << 3)
#define MCHP_VBATR_PFRS_RESETI		(1ul << 4)
#define MCHP_VBATR_PFRS_WDT		(1ul << 5)
#define MCHP_VBATR_PFRS_SYSRESETREQ	(1ul << 6)
#define MCHP_VBATR_PFRS_VBAT_RST	(1ul << 7)

/*
 * Offset 0x08 32K Clock Enable
 */
#define MCHP_VBATR_CLKEN_OFS		0x08ul
#define MCHP_VBATR_CLKEN_MASK		0x0Eul
#define MCHP_VBATR_CLKEN_32K_DOM_POS	1u
#define MCHP_VBATR_CLKEN_32K_SRC_POS	2u
#define MCHP_VBATR_CLKEN_XTAL_SEL_POS	3u

#define MCHP_VBATR_CLKEN_32K_DOM_ALWYS_ON	(0ul << 1)
#define MCHP_VBATR_CLKEN_32K_DOM_32K_IN_PIN	(1ul << 1)
#define MCHP_VBATR_CLKEN_32K_ALWYS_ON_SI_OSC	(0ul << 2)
#define MCHP_VBATR_CLKEN_32K_ALWYS_ON_XTAL	(1ul << 2)
#define MCHP_VBATR_CLKEN_XTAL12_PARALLEL	(0ul << 3)
#define MCHP_VBATR_CLKEN_XTAL2_SE_32K		(1ul << 3)

#define MCHP_VBATR_USE_SIL_OSC		0x00ul
#define MCHP_VBATR_USE_32KIN_PIN	MCHP_VBATR_CLKEN_32K_DOM_32K_IN_PIN
#define MCHP_VBATR_USE_PAR_CRYSTAL \
	(MCHP_VBATR_CLKEN_32K_ALWYS_ON_XTAL + MCHP_VBATR_CLKEN_XTAL12_PARALLEL)
#define MCHP_VBATR_USE_SE_CRYSTAL \
	(MCHP_VBATR_CLKEN_32K_ALWYS_ON_XTAL + MCHP_VBATR_CLKEN_XTAL2_SE_32K)

/*
 * Monotonic Counter
 */
#define MCHP_VBATR_MCNT_LSW_OFS		0x20ul
#define MCHP_VBATR_MCNT_MSW_OFS		0x24ul

/*
 * Register access
 */
#define MCHP_VBATR_PFRS() \
	REG8(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_PFRS_OFS)

#define MCHP_VBATR_CLKEN() \
	REG8(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_CLKEN_OFS)

#define MCHP_VBATR_MCNT_LO() \
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_MCNT_LSW_OFS)

#define MCHP_VBATR_MCNT_HI() \
	REG32(MCHP_VBAT_REGISTERS_ADDR + MCHP_VBATR_MCNT_MSW_OFS)

/* =========================================================================*/
/* ================	       VBATR			   ================ */
/* =========================================================================*/

/**
  * @brief VBAT Register Bank (VBATR)
  */
typedef struct vbatr_regs
 {		/*!< (@ 0x4000A400) VBATR Structure   */
	__IOM uint32_t PFRS;	/*! (@ 0x00000000) VBATR Power Fail Reset Status */
	uint8_t RSVD1[4];
	__IOM uint32_t CLK32_EN;	/*! (@ 0x00000008) VBATR 32K clock enable */
	uint8_t RSVD2[20];
	__IOM uint32_t MCNT_LO;	/*! (@ 0x00000020) VBATR monotonic count lo */
	__IOM uint32_t MCNT_HI;	/*! (@ 0x00000024) VBATR monotonic count hi */
} VBATR_Type;

/* =========================================================================*/
/* ================	       VBATM			   ================ */
/* =========================================================================*/

/**
  * @brief VBAT Memory (VBATM)
  */
#define MCHP_VBAT_MEM_LEN	64ul

typedef struct vbatm_regs
 {		/*!< (@ 0x4000A800) VBATM Structure   */
	union vbmem_u {
		uint32_t u32[(MCHP_VBAT_MEM_LEN) / 4];
		uint16_t u16[(MCHP_VBAT_MEM_LEN) / 2];
		uint8_t u8[MCHP_VBAT_MEM_LEN];
	} MEM;
} VBATM_Type;

#endif	/* #ifndef _VBAT_H */
/* end vbat.h */
/**   @}
 */
