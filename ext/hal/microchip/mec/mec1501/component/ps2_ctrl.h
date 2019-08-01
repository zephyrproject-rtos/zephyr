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

/** @file ps2_ctrl.h
 *MEC1501 PS/2 Controller Registers
 */
/** @defgroup MEC1501 Peripherals PS/2
 */

#ifndef _PS2_CTRL_H
#define _PS2_CTRL_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

/* ===================================================================*/
/* ================		PS2			============= */
/* ===================================================================*/

#define MCHP_PS2_MAX_INSTANCES	2u
#define MCHP_PS2_SPACING	0x40ul
#define MCHP_PS2_SPACING_PWROF2	6u

#define MCHP_PS2_0_BASE_ADDR	0x40009000ul
#define MCHP_PS2_1_BASE_ADDR	0x40009040ul

/*
 * PS2 interrupts
 */
#define MCHP_PS2_0_GIRQ			18u
#define MCHP_PS2_1_GIRQ			18u
#define MCHP_PS2_0_GIRQ_NVIC		10u
#define MCHP_PS2_1_GIRQ_NVIC		10u
#define MCHP_PS2_0_NVIC_DIRECT		100u
#define MCHP_PS2_1_NVIC_DIRECT		101u

#define MCHP_PS2_0_GIRQ_POS	10u
#define MCHP_PS2_1_GIRQ_POS	11u

#define MCHP_PS2_0_GIRQ_VAL	(1ul << 10)
#define MCHP_PS2_1_GIRQ_VAL	(1ul << 11)

/*
 * PS2 TRX Buffer register
 * Writes -> Transmit buffer
 * Read <- Receive buffer
 */
#define MCHP_PS2_TRX_BUFF_REG_MASK	0xFFUL

/*
 * PS2 Control register
 */
#define MCHP_PS2_CTRL_REG_MASK		0x3FUL

/* Select Transmit or Receive */
#define MCHP_PS2_CTRL_TR_POS		0
#define MCHP_PS2_CTRL_TR_RX		(0U << (MCHP_PS2_CTRL_TR_POS))
#define MCHP_PS2_CTRL_TR_TX		(1U << (MCHP_PS2_CTRL_TR_POS))

/* Enable PS2 state machine */
#define MCHP_PS2_CTRL_EN_POS		1
#define MCHP_PS2_CTRL_EN		(1U << (MCHP_PS2_CTRL_EN_POS))

/* Protocol parity selection */
#define MCHP_PS2_CTRL_PAR_POS		2
#define MCHP_PS2_CTRL_PAR_MASK0		0x03U
#define MCHP_PS2_CTRL_PAR_MASK		((MCHP_PS2_CTRL_PAR_MASK0) \
						<< (MCHP_PS2_CTRL_PAR_POS))
#define MCHP_PS2_CTRL_PAR_ODD		(0U << (MCHP_PS2_CTRL_PAR_POS))
#define MCHP_PS2_CTRL_PAR_EVEN		(1U << (MCHP_PS2_CTRL_PAR_POS))
#define MCHP_PS2_CTRL_PAR_IGNORE	(2U << (MCHP_PS2_CTRL_PAR_POS))
#define MCHP_PS2_CTRL_PAR_RSVD		(3U << (MCHP_PS2_CTRL_PAR_POS))

/* Protocol stop bit selection */
#define MCHP_PS2_CTRL_STOP_POS		4
#define MCHP_PS2_CTRL_STOP_MASK0	0x03U
#define MCHP_PS2_CTRL_STOP_MASK		((MCHP_PS2_CTRL_STOP_MASK0) \
						<< (MCHP_PS2_CTRL_STOP_POS))
#define MCHP_PS2_CTRL_STOP_ACT_HI	(0U << (MCHP_PS2_CTRL_STOP_POS))
#define MCHP_PS2_CTRL_STOP_ACT_LO	(1U << (MCHP_PS2_CTRL_STOP_POS))
#define MCHP_PS2_CTRL_STOP_IGNORE	(2U << (MCHP_PS2_CTRL_STOP_POS))
#define MCHP_PS2_CTRL_STOP_RSVD		(3U << (MCHP_PS2_CTRL_STOP_POS))

/*
 * PS2 Status register
 */
#define MCHP_PS2_STATUS_REG_MASK	0xFFUL
#define MCHP_PS2_STATUS_RW1C_MASK	0xAEUL
#define MCHP_PS2_STATUS_RO_MASK		0x51UL
/* RX Data Ready(Read-Only) */
#define MCHP_PS2_STATUS_RXD_RDY_POS	0
#define MCHP_PS2_STATUS_RXD_RDY		(1U << (MCHP_PS2_STATUS_RXD_RDY_POS))
/* RX Timeout(R/W1C) */
#define MCHP_PS2_STATUS_RX_TMOUT_POS	1
#define MCHP_PS2_STATUS_RX_TMOUT	(1U << (MCHP_PS2_STATUS_RX_TMOUT_POS))
/* Parity Error(R/W1C) */
#define MCHP_PS2_STATUS_PE_POS		2
#define MCHP_PS2_STATUS_PE		(1U << (MCHP_PS2_STATUS_PE_POS))
/* Framing Error(R/W1C) */
#define MCHP_PS2_STATUS_FE_POS		3
#define MCHP_PS2_STATUS_FE		(1U << (MCHP_PS2_STATUS_FE_POS))
/* Transmitter is Idle(Read-Only) */
#define MCHP_PS2_STATUS_TX_IDLE_POS	4
#define MCHP_PS2_STATUS_TX_IDLE		(1U << (MCHP_PS2_STATUS_TX_IDLE_POS))
/* Transmitter timeout(R/W1C) */
#define MCHP_PS2_STATUS_TX_TMOUT_POS	5
#define MCHP_PS2_STATUS_TX_TMOUT	(1U << (MCHP_PS2_STATUS_TX_TMOUT_POS))
/* RX is Busy(Read-Only) */
#define MCHP_PS2_STATUS_RX_BUSY_POS	6
#define MCHP_PS2_STATUS_RX_BUSY		(1U << (MCHP_PS2_STATUS_RX_BUSY_POS))
/* Transmitter start timeout(R/W1C) */
#define MCHP_PS2_STATUS_TX_ST_TMOUT_POS	7
#define MCHP_PS2_STATUS_TX_ST_TMOUT	(1U << (MCHP_PS2_STATUS_TX_ST_TMOUT_POS))

/*
 * PS2 Protocol bit positions
 */
#define MCHP_PS2_PROT_START_BIT_POS	1
#define MCHP_PS2_PROT_DATA_BIT0_POS	2
#define MCHP_PS2_PROT_DATA_BIT1_POS	3
#define MCHP_PS2_PROT_DATA_BIT2_POS	4
#define MCHP_PS2_PROT_DATA_BIT3_POS	5
#define MCHP_PS2_PROT_DATA_BIT4_POS	6
#define MCHP_PS2_PROT_DATA_BIT5_POS	7
#define MCHP_PS2_PROT_DATA_BIT6_POS	8
#define MCHP_PS2_PROT_DATA_BIT7_POS	9
#define MCHP_PS2_PROT_PARITY_POS	10
#define MCHP_PS2_PROT_STOP_BIT_POS	11

/**
  * @brief PS/2 Controller Registers (PS2)
  */
typedef struct ps2_regs
{
	__IOM uint32_t TRX_BUFF;	/*!< (@ 0x0000) PS/2 Transmit buffer(WO), Receive buffer(RO) */
	__IOM uint32_t CTRL;		/*!< (@ 0x0004) PS/2 Control */
	__IOM uint32_t STATUS;		/*!< (@ 0x0008) PS/2 Status */
} PS2_Type;

#endif	/* #ifndef _PS2_CTRL_H */
/* end ps2_ctrl.h */
/**   @}
 */
