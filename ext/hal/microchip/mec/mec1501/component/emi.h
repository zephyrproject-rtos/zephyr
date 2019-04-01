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

/** @file emi.h
 *MEC1501 EC Host Memory Interface Registers
 */
/** @defgroup MEC1501 Peripherals EMI
 */

#ifndef _EMI_H
#define _EMI_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   EMI				   ================ */
/* =========================================================================*/

#define MCHP_EMI_BASE_ADDR	0x400F4000ul

#define MCHP_EMI_NUM_INSTANCES	2u
#define MCHP_EMI_SPACING	0x0400ul
#define MCHP_EMI_SPACING_PWROF2	10u

#define MCHP_EMI0_ADDR		0x400F4000ul
#define MCHP_EMI1_ADDR		0x400F4400ul

/*
 * EMI interrupts
 */
#define MCHP_EMI_GIRQ		15u
#define MCHP_EMI_GIRQ_NVIC	7u
/* Direct NVIC connections */
#define MCHP_EMI0_NVIC		42u
#define MCHP_EMI1_NVIC		43u

/* GIRQ Source, Enable_Set/Clr, Result registers bit positions */
#define MCHP_EMI0_GIRQ_POS	2u
#define MCHP_EMI1_GIRQ_POS	3u

#define MCHP_EMI0_GIRQ		(1ul << 2)
#define MCHP_EMI1_GIRQ		(1ul << 3)

/*
 * OS_INT_SRC_LSB
 */
#define MCHP_EMI_OSIS_LSB_EC_WR_POS	0u
#define MCHP_EMI_OSIS_LSB_EC_WR		(1ul << 0)
/* the following bits are also apply to OS_INT_MASK_LSB */
#define MCHP_EMI_OSIS_LSB_SWI_POS	1u
#define MCHP_EMI_OSIS_LSB_SWI_MASK0	0x7Ful
#define MCHP_EMI_OSIS_LSB_SWI_MASK	0xFEul
#define MCHP_EMI_OSIS_LSB_SWI1		(1u << 1)
#define MCHP_EMI_OSIS_LSB_SWI2		(1u << 2)
#define MCHP_EMI_OSIS_LSB_SWI3		(1u << 3)
#define MCHP_EMI_OSIS_LSB_SWI4		(1u << 4)
#define MCHP_EMI_OSIS_LSB_SWI5		(1u << 5)
#define MCHP_EMI_OSIS_LSB_SWI6		(1u << 6)
#define MCHP_EMI_OSIS_LSB_SWI7		(1u << 7)

/*
 * OS_INT_SRC_MSB and OS_INT_MASK_MSB
 */
#define MCHP_EMI_OSIS_MSB_SWI_POS	0u
#define MCHP_EMI_OSIS_MSB_SWI_MASK0	0xFFul
#define MCHP_EMI_OSIS_MSB_SWI_MASK	0xFFul
#define MCHP_EMI_OSIS_MSB_SWI8		(1u << 0)
#define MCHP_EMI_OSIS_MSB_SWI9		(1u << 1)
#define MCHP_EMI_OSIS_MSB_SWI10		(1u << 2)
#define MCHP_EMI_OSIS_MSB_SWI11		(1u << 3)
#define MCHP_EMI_OSIS_MSB_SWI12		(1u << 4)
#define MCHP_EMI_OSIS_MSB_SWI13		(1u << 5)
#define MCHP_EMI_OSIS_MSB_SWI14		(1u << 6)
#define MCHP_EMI_OSIS_MSB_SWI15		(1u << 7)

/*
 * OS_APP_ID
 */
#define MCHP_EMI_OS_APP_ID_MASK		0xFFul

/*
 * MEM_BASE_0 and MEM_BASE_1 registers
 * bits[1:0] = 00b read-only forcing EC SRAM location to
 * be aligned >= 4 bytes.
 */
#define MCHP_EMI_MEM_BASE_MASK		0xFFFFFFFCul

/*
 * MEM_LIMIT_0 and MEM_LIMIT_1 registers are split into two fields
 * bits[15:0] = read limit
 *	bits[1:0]=00b read-only forcing >= 4 byte alignment
 * bits[31:16] = write limit
 *	bits[17:16]=00b read-only forcing >= 4 byte alignment
 */
#define MEM_EMI_MEM_LIMIT_MASK		0xFFFCFFFCul
#define MEM_EMI_MEM_LIMIT_RD_POS	0u
#define MEM_EMI_MEM_LIMIT_RD_MASK0	0xFFFCul
#define MEM_EMI_MEM_LIMIT_RD_MASK	0xFFFCul
#define MEM_EMI_MEM_LIMIT_WR_POS	16u
#define MEM_EMI_MEM_LIMIT_WR_MASK0	0xFFFCul
#define MEM_EMI_MEM_LIMIT_WR_MASK	(0xFFFCul << 16)

/*
 * EC_SET_OS_INT and EC_OS_INT_CLR_EN
 */
#define MCHP_EMI_EC_OS_SWI_MASK	0xFFFEul
#define MCHP_EMI_EC_OS_SWI1	(1ul << 1)
#define MCHP_EMI_EC_OS_SWI2	(1ul << 2)
#define MCHP_EMI_EC_OS_SWI3	(1ul << 3)
#define MCHP_EMI_EC_OS_SWI4	(1ul << 4)
#define MCHP_EMI_EC_OS_SWI5	(1ul << 5)
#define MCHP_EMI_EC_OS_SWI6	(1ul << 6)
#define MCHP_EMI_EC_OS_SWI7	(1ul << 7)
#define MCHP_EMI_EC_OS_SWI8	(1ul << 8)
#define MCHP_EMI_EC_OS_SWI9	(1ul << 9)
#define MCHP_EMI_EC_OS_SWI10	(1ul << 10)
#define MCHP_EMI_EC_OS_SWI11	(1ul << 11)
#define MCHP_EMI_EC_OS_SWI12	(1ul << 12)
#define MCHP_EMI_EC_OS_SWI13	(1ul << 13)
#define MCHP_EMI_EC_OS_SWI14	(1ul << 14)
#define MCHP_EMI_EC_OS_SWI15	(1ul << 15)


/**
  * @brief EMI Registers (EMI)
  */
typedef struct emi_regs
{
	__IOM uint8_t OS_H2E_MBOX;	/*!< (@ 0x0000) OS space Host to EC mailbox register */
	__IOM uint8_t OS_E2H_MBOX;	/*!< (@ 0x0001) OS space EC to Host mailbox register */
	__IOM uint8_t OS_EC_ADDR_LSB;	/*!< (@ 0x0002) OS space EC memory address LSB register */
	__IOM uint8_t OS_EC_ADDR_MSB;	/*!< (@ 0x0003) OS space EC memory address LSB register */
	__IOM uint32_t OS_EC_DATA;	/*!< (@ 0x0004) OS space EC Data register */
	__IOM uint8_t OS_INT_SRC_LSB;	/*!< (@ 0x0008) OS space Interrupt Source LSB register */
	__IOM uint8_t OS_INT_SRC_MSB;	/*!< (@ 0x0009) OS space Interrupt Source MSB register */
	__IOM uint8_t OS_INT_MASK_LSB;	/*!< (@ 0x000A) OS space Interrupt Mask LSB register */
	__IOM uint8_t OS_INT_MASK_MSB;	/*!< (@ 0x000B) OS space Interrupt Mask MSB register */
	__IOM uint32_t OS_APP_ID;	/*!< (@ 0x000C) OS space Application ID register */
	uint8_t RSVD1[0x100u - 0x10u];
	__IOM uint8_t H2E_MBOX;		/*!< (@ 0x0100) Host to EC mailbox */
	__IOM uint8_t E2H_MBOX;		/*!< (@ 0x0101) EC Data */
	uint8_t RSVD2[2];
	__IOM uint32_t MEM_BASE_0;	/*!< (@ 0x0104) EC memory region 0 base address */
	__IOM uint32_t MEM_LIMIT_0;	/*!< (@ 0x0108) EC memory region 0 read/write limits */
	__IOM uint32_t MEM_BASE_1;	/*!< (@ 0x010C) EC memory region 1 base address */
	__IOM uint32_t MEM_LIMIT_1;	/*!< (@ 0x0110) EC memory region 1 read/write limits */
	__IOM uint16_t EC_OS_INT_SET;	/*!< (@ 0x0114) Set OS interrupt source */
	__IOM uint16_t EC_OS_INT_CLR_EN;	/*!< (@ 0x0116) OS interrupt source clear enable */
} EMI_Type;

#endif	/* #ifndef _EMI_H */
/* end emi.h */
/**   @}
 */
