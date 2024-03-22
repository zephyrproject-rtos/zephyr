/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PS2_H
#define _MEC_PS2_H

#include <stdint.h>
#include <stddef.h>

/*
 * PS2 TRX Buffer register
 * Writes -> Transmit buffer
 * Read <- Receive buffer
 */
#define MCHP_PS2_TRX_BUFF_REG_MASK	0xffu

/* PS2 Control register */
#define MCHP_PS2_CTRL_REG_MASK		0x3fu

/* Select Transmit or Receive */
#define MCHP_PS2_CTRL_TR_POS		0u
#define MCHP_PS2_CTRL_TR_RX		0u
#define MCHP_PS2_CTRL_TR_TX		BIT(MCHP_PS2_CTRL_TR_POS)

/* Enable PS2 state machine */
#define MCHP_PS2_CTRL_EN_POS		1u
#define MCHP_PS2_CTRL_EN		BIT(MCHP_PS2_CTRL_EN_POS)

/* Protocol parity selection */
#define MCHP_PS2_CTRL_PAR_POS		2u
#define MCHP_PS2_CTRL_PAR_MASK0		0x03u
#define MCHP_PS2_CTRL_PAR_MASK		0x0cu
#define MCHP_PS2_CTRL_PAR_ODD		0u
#define MCHP_PS2_CTRL_PAR_EVEN		0x04u
#define MCHP_PS2_CTRL_PAR_IGNORE	0x08u
#define MCHP_PS2_CTRL_PAR_RSVD		0x0cu

/* Protocol stop bit selection */
#define MCHP_PS2_CTRL_STOP_POS		4u
#define MCHP_PS2_CTRL_STOP_MASK0	0x03u
#define MCHP_PS2_CTRL_STOP_MASK		0x30u
#define MCHP_PS2_CTRL_STOP_ACT_HI	0u
#define MCHP_PS2_CTRL_STOP_ACT_LO	0x10u
#define MCHP_PS2_CTRL_STOP_IGNORE	0x20u
#define MCHP_PS2_CTRL_STOP_RSVD		0x30u

/* PS2 Status register */
#define MCHP_PS2_STATUS_REG_MASK	0xffu
#define MCHP_PS2_STATUS_RW1C_MASK	0xaeu
#define MCHP_PS2_STATUS_RO_MASK		0x51u
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

/** @brief PS/2 controller */
struct ps2_regs {
	volatile uint32_t TRX_BUFF;
	volatile uint32_t CTRL;
	volatile uint32_t STATUS;
};

#endif	/* #ifndef _MEC_PS2_H */
