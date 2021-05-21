/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PS2_CTRL_H
#define _MEC_PS2_CTRL_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_PS2_0_BASE_ADDR	0x40009000ul

/* PS2 interrupts */
#define MCHP_PS2_0_GIRQ			18u
#define MCHP_PS2_0_GIRQ_NVIC		10u
#define MCHP_PS2_0_NVIC_DIRECT		100u

#define MCHP_PS2_0_GIRQ_POS	10u
#define MCHP_PS2_0_GIRQ_VAL	BIT(10)

/* PS2 wake on port activity interrupts */
#define MCHP_PS2_0_WK_GIRQ		21u
#define MCHP_PS2_0_0A_WK_GIRQ_VAL	BIT(18)
#define MCHP_PS2_0_0B_WK_GIRQ_VAL	BIT(19)

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
#define MCHP_PS2_CTRL_TR_RX		0U
#define MCHP_PS2_CTRL_TR_TX		BIT(MCHP_PS2_CTRL_TR_POS)

/* Enable PS2 state machine */
#define MCHP_PS2_CTRL_EN_POS		1
#define MCHP_PS2_CTRL_EN		BIT(MCHP_PS2_CTRL_EN_POS)

/* Protocol parity selection */
#define MCHP_PS2_CTRL_PAR_POS		2
#define MCHP_PS2_CTRL_PAR_MASK0		0x03U
#define MCHP_PS2_CTRL_PAR_MASK		0x0CU
#define MCHP_PS2_CTRL_PAR_ODD		0U
#define MCHP_PS2_CTRL_PAR_EVEN		0x04U
#define MCHP_PS2_CTRL_PAR_IGNORE	0x08U
#define MCHP_PS2_CTRL_PAR_RSVD		0x0CU

/* Protocol stop bit selection */
#define MCHP_PS2_CTRL_STOP_POS		4
#define MCHP_PS2_CTRL_STOP_MASK0	0x03U
#define MCHP_PS2_CTRL_STOP_MASK		0x30U
#define MCHP_PS2_CTRL_STOP_ACT_HI	0U
#define MCHP_PS2_CTRL_STOP_ACT_LO	0x10U
#define MCHP_PS2_CTRL_STOP_IGNORE	0x20U
#define MCHP_PS2_CTRL_STOP_RSVD		0x30U

/* PS2 Status register */
#define MCHP_PS2_STATUS_REG_MASK	0xFFUL
#define MCHP_PS2_STATUS_RW1C_MASK	0xAEUL
#define MCHP_PS2_STATUS_RO_MASK		0x51UL
/* RX Data Ready(Read-Only) */
#define MCHP_PS2_STATUS_RXD_RDY_POS	0
#define MCHP_PS2_STATUS_RXD_RDY		BIT(MCHP_PS2_STATUS_RXD_RDY_POS)
/* RX Timeout(R/W1C) */
#define MCHP_PS2_STATUS_RX_TMOUT_POS	1
#define MCHP_PS2_STATUS_RX_TMOUT	BIT(MCHP_PS2_STATUS_RX_TMOUT_POS)
/* Parity Error(R/W1C) */
#define MCHP_PS2_STATUS_PE_POS		2
#define MCHP_PS2_STATUS_PE		BIT(MCHP_PS2_STATUS_PE_POS)
/* Framing Error(R/W1C) */
#define MCHP_PS2_STATUS_FE_POS		3
#define MCHP_PS2_STATUS_FE		BIT(MCHP_PS2_STATUS_FE_POS)
/* Transmitter is Idle(Read-Only) */
#define MCHP_PS2_STATUS_TX_IDLE_POS	4
#define MCHP_PS2_STATUS_TX_IDLE		BIT(MCHP_PS2_STATUS_TX_IDLE_POS)
/* Transmitter timeout(R/W1C) */
#define MCHP_PS2_STATUS_TX_TMOUT_POS	5
#define MCHP_PS2_STATUS_TX_TMOUT	BIT(MCHP_PS2_STATUS_TX_TMOUT_POS)
/* RX is Busy(Read-Only) */
#define MCHP_PS2_STATUS_RX_BUSY_POS	6
#define MCHP_PS2_STATUS_RX_BUSY		BIT(MCHP_PS2_STATUS_RX_BUSY_POS)
/* Transmitter start timeout(R/W1C) */
#define MCHP_PS2_STATUS_TX_ST_TMOUT_POS 7
#define MCHP_PS2_STATUS_TX_ST_TMOUT	BIT(MCHP_PS2_STATUS_TX_ST_TMOUT_POS)

/* PS2 Protocol bit positions */
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

#endif	/* #ifndef _MEC_PS2_CTRL_H */
