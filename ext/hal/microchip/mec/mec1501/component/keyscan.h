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

/** @file keyscan.h
 *MEC1501 Keyboard matrix scan controller registers
 */
/** @defgroup MEC1501 Peripherals KSCAN
 */

#ifndef _KSCAN_H
#define _KSCAN_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   KBC				   ================ */
/* =========================================================================*/

#define MCHP_KSCAN_BASE_ADDR	0x40009C00ul

/*
 * KSCAN interrupts
 */
#define MCHP_KSCAN_GIRQ		21u
#define MCHP_KSCAN_GIRQ_NVIC	13u
/* Direct mode connection to NVIC */
#define MCHP_KSAN_NVIC		135u

#define MCHP_KSCAN_GIRQ_POS	25u

#define MCHP_KSCAN_GIRQ_VAL	(1ul << 25)

/*
 * KSO_SEL
 */
#define MCHP_KSCAN_KSO_SEL_REG_MASK	0xFFul
#define MCHP_KSCAN_KSO_LINES_POS	0u
#define MCHP_KSCAN_KSO_LINES_MASK0	0x1Ful
#define MCHP_KSCAN_KSO_LINES_MASK	0x1Ful
#define MCHP_KSCAN_KSO_ALL_POS		5u
#define MCHP_KSCAN_KSO_ALL		(1ul << 5)
#define MCHP_KSCAN_KSO_EN_POS		6u
#define MCHP_KSCAN_KSO_EN		(1ul << 6)
#define MCHP_KSCAN_KSO_INV_POS		7u
#define MCHP_KSCAN_KSO_INV		(1ul << 7)

/*
 * KSI_IN
 */
#define MCHP_KSCAN_KSI_IN_REG_MASK	0xFFul

/*
 * KSI_STS
 */
#define MCHP_KSCAN_KSI_STS_REG_MASK	0xFFul

/*
 * KSI_IEN
 */
#define MCHP_KSCAN_KSI_IEN_REG_MASK	0xFFul

/*
 * EXT_CTRL
 */
#define MCHP_KSCAN_EXT_CTRL_REG_MASK	0x01ul
#define MCHP_KSCAN_EXT_CTRL_PREDRV_EN	0x01ul

/**
  * @brief Keyboard Controller Registers (KSCAN)
  */
typedef struct kscan_regs
{
	uint8_t RSVD[4];
	__IOM uint32_t KSO_SEL;	/*!< (@ 0x0004) KSO Select */
	uint32_t KSI_IN;	/*!< (@ 0x0008) KSI Input States */
	__IOM uint32_t KSI_STS;	/*!< (@ 0x000C) KSI Status */
	__IOM uint32_t KSI_IEN;	/*!< (@ 0x0010) KSI Interrupt Enable */
	__IOM uint32_t EXT_CTRL;	/*!< (@ 0x0014) Extended Control */
} KSCAN_Type;

#endif	/* #ifndef _KSCAN_H */
/* end kscan.h */
/**   @}
 */
