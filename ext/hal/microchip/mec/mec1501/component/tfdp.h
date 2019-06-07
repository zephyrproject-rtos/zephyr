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

/** @file tfdp.h
 *MEC1501 TFDP Trace FIFO Debug Port Registers
 */
/** @defgroup MEC1501 Peripherals TFDP
 */

#ifndef _TFDP_H
#define _TFDP_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================		TFDP			=================== */
/* =========================================================================*/

#define MCHP_TFDP_BASE_ADDR	0x40008c00ul

#define MCHP_TFDP_DATA_OUT_ADDR	((MCHP_TFDP_BASE_ADDR) + 0)
#define MCHP_TFDP_CTRL_ADDR	((MCHP_TFDP_BASE_ADDR) + 4ul)

#define MCHP_TFDP_CTRL_REG_MASK		0x7Ful
#define MCHP_TFDP_CTRL_EN_POS		0u
#define MCHP_TFDP_CTRL_EDGE_SEL_POS	1u
#define MCHP_TFDP_CTRL_DIV_SEL_POS	2u
#define MCHP_TFDP_CTRL_IP_DLY_POS	4u

#define MCHP_TFDP_CTRL_EN		(1ul << 0)

#define MCHP_TFDP_OUT_ON_RISING_EDGE	(0ul << 1)
#define MCHP_TFDP_OUT_ON_FALLING_EDGE	(1ul << 1)
#define MCHP_TFDP_CLK_AHB_DIV_2		(0ul << 2)
#define MCHP_TFDP_CLK_AHB_DIV_4		(1ul << 2)
#define MCHP_TFDP_CLK_AHB_DIV_8		(2ul << 2)
#define MCHP_TFDP_CLK_AHB_DIV_2_ALT	(3ul << 2)

/* Number of AHB clocks between each byte shifted out */
#define MCHP_TFDP_IP_DLY_1		(0ul << 4)
#define MCHP_TFDP_IP_DLY_2		(1ul << 4)
#define MCHP_TFDP_IP_DLY_3		(2ul << 4)
#define MCHP_TFDP_IP_DLY_4		(3ul << 4)
#define MCHP_TFDP_IP_DLY_5		(4ul << 4)
#define MCHP_TFDP_IP_DLY_6		(5ul << 4)
#define MCHP_TFDP_IP_DLY_7		(6ul << 4)
#define MCHP_TFDP_IP_DLY_8		(7ul << 4)

/* First byte indicates start of packet */
#define MCHP_TFDP_PKT_START		0xFDu

/**
  * @brief Trace FIFO Debug Port Registers (TFDP)
  */
typedef struct tfdp_regs
{
	__IOM uint8_t  DATA_OUT;	/*!< (@ 0x0000) Data out shift register */
	uint8_t RSVD1[3];
	__IOM uint32_t  CTRL;		/*!< (@ 0x0004) Control register */
} TFDP_Type;

#endif	/* #ifndef _TFDP_H */
/* end tfdp.h */
/**   @}
 */
