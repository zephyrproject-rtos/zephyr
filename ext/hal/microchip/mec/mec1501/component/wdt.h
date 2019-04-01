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

/** @file wdt.h
 *MEC1501 Watch Dog Timer Registers
 */
/** @defgroup MEC1501 Peripherals WDT
 */

#ifndef _WDT_H
#define _WDT_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   WDT				   ================ */
/* =========================================================================*/

#define MCHP_WDT_BASE_ADDR		0x40000400ul

#define MCHP_WDT_CTRL_MASK		0x021Dul
#define MCHP_WDT_CTRL_EN_POS		0u
#define MCHP_WDT_CTRL_EN_MASK		(1u1 << (MCHP_WDT_CTRL_EN_POS))
#define MCHP_WDT_CTRL_EN		(1u1 << (MCHP_WDT_CTRL_EN_POS))
#define MCHP_WDT_CTRL_HTMR_STALL_POS	2u
#define MCHP_WDT_CTRL_HTMR_STALL_MASK	(1u1 << (MCHP_WDT_CTRL_HTMR_STALL_POS))
#define MCHP_WDT_CTRL_HTMR_STALL_EN	(1u1 << (MCHP_WDT_CTRL_HTMR_STALL_POS))
#define MCHP_WDT_CTRL_WKTMR_STALL_POS	3u
#define MCHP_WDT_CTRL_WKTMR_STALL_MASK	(1u1 << (MCHP_WDT_CTRL_WKTMR_STALL_POS))
#define MCHP_WDT_CTRL_WKTMR_STALL_EN	(1u1 << (MCHP_WDT_CTRL_WKTMR_STALL_POS))
#define MCHP_WDT_CTRL_JTAG_STALL_POS	4u
#define MCHP_WDT_CTRL_JTAG_STALL_MASK	(1u1 << (MCHP_WDT_CTRL_JTAG_STALL_POS))
#define MCHP_WDT_CTRL_JTAG_STALL_EN	(1u1 << (MCHP_WDT_CTRL_JTAG_STALL_POS))

/* WDT Kick register. Write any value to reload counter */
#define MCHP_WDT_KICK_OFS		0x08ul

/* WDT Count register. Read only */
#define MCHP_WDT_CNT_RO_OFS		0x0Cul
#define MCHP_WDT_CNT_RO_MASK		0xFFFFul

/*
 * If this bit is set when the WDT counts down it will clear this
 * bit, fire an interrupt if IEN is enabled, and start counting up.
 * Once it reaches maximum count it actives its reset output.
 * This feature allows WDT ISR time to take action before WDT asserts
 * its reset signal.
 * If this bit is clear WDT will immediately assert its reset signal
 * when counter counts down to 0.
 */
#define WDT_CTRL_INH1_POS	9u
#define WDT_CTRL_INH1_MASK	(1u1 << (WDT_CTRL_INH1_POS))
#define WDT_CTRL_INH1_EN	(1u1 << (WDT_CTRL_INH1_POS))

/* Interrupt Enable Register */
#define WDT_IEN_MASK		0x01ul
#define WDT_IEN_EVENT_IRQ_POS	0u
#define WDT_IEN_EVENT_IRQ_MASK	(1ul << (WDT_IEN_EVENT_IRQ_POS))
#define WDT_IEN_EVENT_IRQ_EN	(1ul << (WDT_IEN_EVENT_IRQ_POS))

/**
  * @brief Watch Dog Timer (WDT)
  */
typedef struct wdt_regs
 {
	__IOM uint16_t LOAD;	/*!< (@ 0x00000000) WDT Load	 */
	uint8_t RSVD1[2];
	__IOM uint16_t CTRL;	/*!< (@ 0x00000004) WDT Control	 */
	uint8_t RSVD2[2];
	__OM uint8_t KICK;	/*!< (@ 0x00000008) WDT Kick (WO) */
	uint8_t RSVD3[3];
	__IM uint16_t CNT;	/*!< (@ 0x0000000C) WDT Count (RO)  */
	uint8_t RSVD4[2];
	__IOM uint16_t STS;	/*!< (@ 0x00000010) WDT Status	*/
	uint8_t RSVD5[2];
	__IOM uint8_t IEN;	/*!< (@ 0x00000010) WDT Interrupt Enable  */
} WDT_Type;

#endif	/* #ifndef _WDT_H */
/* end wdt.h */
/**   @}
 */
