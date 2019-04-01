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

/** @file mailbox.h
 *MEC1501 Mailbox Registers
 */
/** @defgroup MEC1501 Peripherals MBOX
 */

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* =========================================================================*/
/* ================   MAILBOX				   ================ */
/* =========================================================================*/

#define MCHP_MBOX_BASE_ADDR	0x400F0000ul

/*
 * KBC interrupts
 */
#define MCHP_MBOX_GIRQ		15u
#define MCHP_MBOX_GIRQ_NVIC	7u
#define MCHP_MBOX_NVIC_DIRECT	60u

#define MCHP_KBC_MBOX_GIRQ_POS	20u
#define MCHP_KBC_MBOX_GIRQ	(1ul << 20u)

/*
 * SMI Source register
 */
#define MCHP_MBOX_SMI_SRC_EC_WR_POS	0u
#define MCHP_MBOX_SMI_SRC_EC_WR		(1ul << (MCHP_MBOX_SMI_SRC_WR_POS))
#define MCHP_MBOX_SMI_SRC_SWI_POS	1u
#define MCHP_MBOX_SMI_SRC_SWI_MASK0	0x7Ful
#define MCHP_MBOX_SMI_SRC_SWI_MASK	0xFEul
#define MCHP_MBOX_SMI_SRC_SWI0		(1ul << 1)
#define MCHP_MBOX_SMI_SRC_SWI1		(1ul << 2)
#define MCHP_MBOX_SMI_SRC_SWI2		(1ul << 3)
#define MCHP_MBOX_SMI_SRC_SWI3		(1ul << 4)
#define MCHP_MBOX_SMI_SRC_SWI4		(1ul << 5)
#define MCHP_MBOX_SMI_SRC_SWI5		(1ul << 6)
#define MCHP_MBOX_SMI_SRC_SWI6		(1ul << 7)

/*
 * SMI Mask register
 */
#define MCHP_MBOX_SMI_MASK_WR_EN_POS	0u
#define MCHP_MBOX_SMI_MASK_WR_EN	(1ul << (MCHP_MBOX_SMI_MASK_WR_EN_POS))
#define MCHP_MBOX_SMI_SWI_EN_POS	1u
#define MCHP_MBOX_SMI_SWI_EN_MASK0	0x7Ful
#define MCHP_MBOX_SMI_SWI_EN_MASK	0xFEul
#define MCHP_MBOX_SMI_SRC_EN_SWI0	(1ul << 1)
#define MCHP_MBOX_SMI_SRC_EN_SWI1	(1ul << 2)
#define MCHP_MBOX_SMI_SRC_EN_SWI2	(1ul << 3)
#define MCHP_MBOX_SMI_SRC_EN_SWI3	(1ul << 4)
#define MCHP_MBOX_SMI_SRC_EN_SWI4	(1ul << 5)
#define MCHP_MBOX_SMI_SRC_EN_SWI5	(1ul << 6)
#define MCHP_MBOX_SMI_SRC_EN_SWI6	(1ul << 7)

/**
  * @brief Mailbox Registers (MBOX)
  */
typedef struct mbox_regs
{
	__IOM uint8_t OS_IDX;		/*!< (@ 0x0000) OS Index */
	__IOM uint8_t OS_DATA;		/*!< (@ 0x0001) OS Data */
	uint8_t RSVD1[0x100u - 0x02u];
	__IOM uint32_t HOST_TO_EC;	/*!< (@ 0x0100) Host to EC */
	__IOM uint32_t EC_TO_HOST;	/*!< (@ 0x0104) EC to Host */
	__IOM uint32_t SMI_SRC;		/*!< (@ 0x0108) SMI Source */
	__IOM uint32_t SMI_MASK;	/*!< (@ 0x010C) SMI Mask */
	__IOM uint32_t MBX_0_3;		/*!< (@ 0x0110) Mailboxes 0 - 3 */
	__IOM uint32_t MBX_4_7;		/*!< (@ 0x0114) Mailboxes 4 - 7 */
	__IOM uint32_t MBX_8_11;	/*!< (@ 0x0118) Mailboxes 8 - 11 */
	__IOM uint32_t MBX_12_15;	/*!< (@ 0x011C) Mailboxes 12 - 15 */
	__IOM uint32_t MBX_16_19;	/*!< (@ 0x0120) Mailboxes 16 - 19 */
	__IOM uint32_t MBX_20_23;	/*!< (@ 0x0124) Mailboxes 20 - 23 */
	__IOM uint32_t MBX_24_27;	/*!< (@ 0x0128) Mailboxes 24 - 27 */
	__IOM uint32_t MBX_28_31;	/*!< (@ 0x012C) Mailboxes 28 - 31 */
} MBOX_Type;

#endif	/* #ifndef _MAILBOX_H */
/* end mailbox.h */
/**   @}
 */
