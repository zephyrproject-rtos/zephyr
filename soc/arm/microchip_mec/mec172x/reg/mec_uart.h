/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_UART_H
#define _MEC_UART_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

#define MCHP_UART0_ID		0u
#define MCHP_UART1_ID		1u
#define MCHP_UART_MAX_ID	2u

#define MCHP_UART_RX_FIFO_MAX_LEN	16u
#define MCHP_UART_TX_FIFO_MAX_LEN	16u

#define MCHP_UART_BAUD_RATE_MIN		50ul
#define MCHP_UART_BAUD_RATE_MAX		1500000ul

#define MCHP_UART0_BASE_ADDRESS		0x400F2400ul
#define MCHP_UART1_BASE_ADDRESS		0x400F2800ul
#define MCHP_UART_SPACING		0x400ul

#define MCHP_UART_BASE_ADDR(n) \
	(MCHP_UART0_BASE_ADDRESS + ((uint32_t)(n) * 0x400ul))

#define MCHP_UART_GIRQ_NUM		15u
#define MCHP_UART_GIRQ_ID		7u
#define MCHP_UART0_GIRQ_POS		0u
#define MCHP_UART1_GIRQ_POS		1u
#define MCHP_UART0_GIRQ_VAL		BIT(MCHP_UART0_GIRQ_POS)
#define MCHP_UART1_GIRQ_VAL		BIT(MCHP_UART1_GIRQ_POS)
#define MCHP_UART0_NVIC			40u
#define MCHP_UART1_NVIC			41u
#define MCHP_UART2_NVIC			44u

/*
 * LCR DLAB=0
 * Transmit buffer(WO), Receive buffer(RO)
 * LCR DLAB=1, BAUD rate divisor LSB
 */
#define MCHP_UART_RTXB_OFS	0u
#define MCHP_UART_BRGD_LSB_OFS	0u

/*
 * LCR DLAB=0
 * Interrupt Enable Register, R/W
 * LCR DLAB=1, BAUD rate divisor MSB
 */
#define MCHP_UART_BRGD_MSB_OFS	1u
#define MCHP_UART_IER_OFS	1u
#define MCHP_UART_IER_MASK	0x0Ful
#define MCHP_UART_IER_ERDAI	0x01ul	/* Received data available and timeouts */
#define MCHP_UART_IER_ETHREI	0x02ul	/* TX Holding register empty */
#define MCHP_UART_IER_ELSI	0x04ul	/* Errors: Overrun, Parity, Framing, and Break */
#define MCHP_UART_IER_EMSI	0x08ul	/* Modem Status */
#define MCHP_UART_IER_ALL	0x0Ful

/* FIFO Contro Register, Write-Only */
#define MCHP_UART_FCR_OFS		2u
#define MCHP_UART_FCR_MASK		0xCFu
#define MCHP_UART_FCR_EXRF		0x01u	/* Enable TX & RX FIFO's */
#define MCHP_UART_FCR_CLR_RX_FIFO	0x02u	/* Clear RX FIFO, bit is self-clearing */
#define MCHP_UART_FCR_CLR_TX_FIFO	0x04u	/* Clear TX FIFO, bit is self-clearing */
#define MCHP_UART_FCR_DMA_EN		0x08u	/* DMA Mode Enable. Not implemented */
#define MCHP_UART_FCR_RX_FIFO_LVL_MASK	0xC0u	/* RX FIFO trigger level mask */
#define MCHP_UART_FCR_RX_FIFO_LVL_1	0x00u
#define MCHP_UART_FCR_RX_FIFO_LVL_4	0x40u
#define MCHP_UART_FCR_RX_FIFO_LVL_8	0x80u
#define MCHP_UART_FCR_RX_FIFO_LVL_14	0xC0u

/* Interrupt Identification Register, Read-Only */
#define MCHP_UART_IIR_OFS		2u
#define MCHP_UART_IIR_MASK		0xCFu
#define MCHP_UART_IIR_NOT_IPEND		0x01u
#define MCHP_UART_IIR_INTID_MASK0	0x07u
#define MCHP_UART_IIR_INTID_POS		1u
#define MCHP_UART_IIR_INTID_MASK	0x0Eu
#define MCHP_UART_IIR_FIFO_EN_MASK	0xC0u
/*
 * interrupt values
 * Highest priority: Line status, overrun, framing, or break
 * Highest-1. RX data available or RX FIFO trigger level reached
 * Highest-2. RX timeout
 * Highest-3. TX Holding register empty
 * Highest-4. MODEM status
 */
#define MCHP_UART_IIR_INT_NONE		0x01u
#define MCHP_UART_IIR_INT_LS		0x06u
#define MCHP_UART_IIR_INT_RX		0x04u
#define MCHP_UART_IIR_INT_RX_TMOUT	0x0Cu
#define MCHP_UART_IIR_INT_THRE		0x02u
#define MCHP_UART_IIR_INT_MS		0x00u

/* Line Control Register R/W */
#define MCHP_UART_LCR_OFS		3u
#define MCHP_UART_LCR_WORD_LEN_MASK	0x03u
#define MCHP_UART_LCR_WORD_LEN_5	0x00u
#define MCHP_UART_LCR_WORD_LEN_6	0x01u
#define MCHP_UART_LCR_WORD_LEN_7	0x02u
#define MCHP_UART_LCR_WORD_LEN_8	0x03u
#define MCHP_UART_LCR_STOP_BIT_1	0x00u
#define MCHP_UART_LCR_STOP_BIT_2	0x04u	/* 2 for 6-8 bits or 1.5 for 5 bits */
#define MCHP_UART_LCR_PARITY_NONE	0x00u
#define MCHP_UART_LCR_PARITY_EN		0x08u
#define MCHP_UART_LCR_PARITY_ODD	0x00u
#define MCHP_UART_LCR_PARITY_EVEN	0x10u
#define MCHP_UART_LCR_STICK_PARITY	0x20u
#define MCHP_UART_LCR_BREAK_EN		0x40u
#define MCHP_UART_LCR_DLAB_EN		0x80u

/* MODEM Control Register R/W */
#define MCHP_UART_MCR_OFS		4u
#define MCHP_UART_MCR_MASK		0x1Fu
#define MCHP_UART_MCR_DTRn		0x01u
#define MCHP_UART_MCR_RTSn		0x02u
#define MCHP_UART_MCR_OUT1		0x04u
#define MCHP_UART_MCR_OUT2		0x08u
#define MCHP_UART_MCR_LOOPBCK_EN	0x10u

/* Line Status Register RO */
#define MCHP_UART_LSR_OFS		5u
#define MCHP_UART_LSR_DATA_RDY		0x01u
#define MCHP_UART_LSR_OVERRUN		0x02u
#define MCHP_UART_LSR_PARITY		0x04u
#define MCHP_UART_LSR_FRAME		0x08u
#define MCHP_UART_LSR_RX_BREAK		0x10u
#define MCHP_UART_LSR_THRE		0x20u
#define MCHP_UART_LSR_TEMT		0x40u
#define MCHP_UART_LSR_FIFO_ERR		0x80u
#define MCHP_UART_LSR_ANY		0xFFu

/* MODEM Status Register RO */
#define MCHP_UART_MSR_OFS		6u
#define MCHP_UART_MSR_DCTS		0x01u
#define MCHP_UART_MSR_DDSR		0x02u
#define MCHP_UART_MSR_TERI		0x04u
#define MCHP_UART_MSR_DDCD		0x08u
#define MCHP_UART_MSR_CTS		0x10u
#define MCHP_UART_MSR_DSR		0x20u
#define MCHP_UART_MSR_RI		0x40u
#define MCHP_UART_MSR_DCD		 0x80u

/* Scratch Register RO */
#define MCHP_UART_SCR_OFS	7u

/* UART Logical Device Activate Register */
#define MCHP_UART_LD_ACT	0x330ul
#define MCHP_UART_LD_ACTIVATE	0x01ul

/* UART Logical Device Config Register */
#define MCHP_UART_LD_CFG		0x3F0ul
#define MCHP_UART_LD_CFG_INTCLK		0U
#define MCHP_UART_LD_CFG_NO_INVERT	0U
#define MCHP_UART_LD_CFG_RESET_SYS	0U
#define MCHP_UART_LD_CFG_EXTCLK		BIT(0)
#define MCHP_UART_LD_CFG_RESET_VCC	BIT(1)
#define MCHP_UART_LD_CFG_INVERT		BIT(2)

/* BAUD rate generator */
#define MCHP_UART_INT_CLK_24M		BIT(15)

/* 1.8MHz internal clock source */
#define MCHP_UART_1P8M_BAUD_50		2304u
#define MCHP_UART_1P8M_BAUD_110		1536u
#define MCHP_UART_1P8M_BAUD_150		768u
#define MCHP_UART_1P8M_BAUD_300		384u
#define MCHP_UART_1P8M_BAUD_1200	96u
#define MCHP_UART_1P8M_BAUD_2400	48u
#define MCHP_UART_1P8M_BAUD_9600	12u
#define MCHP_UART_1P8M_BAUD_19200	6u
#define MCHP_UART_1P8M_BAUD_38400	3u
#define MCHP_UART_1P8M_BAUD_57600	2u
#define MCHP_UART_1P8M_BAUD_115200	1u

/* 24MHz internal clock source. n = 24e6 / (BAUD * 16) = 1500000 / BAUD */
#define MCHP_UART_24M_BAUD_115200	((13u) + (MCHP_UART_INT_CLK_24M))
#define MCHP_UART_24M_BAUD_57600	((26u) + (MCHP_UART_INT_CLK_24M))

/*
 * Register access by UART zero based ID
 * 0 <= uart id <= 1
 */
#define MCHP_UART_TXB_WO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_RTXB_OFS)
#define MCHP_UART_RXB_RO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_RTXB_OFS)
#define MCHP_UART_BRGD_LSB_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_BRGD_LSB_OFS)
#define MCHP_UART_BRGD_MSB_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_BRGD_MSB_OFS)
#define MCHP_UART_FCR_WO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_FCR_OFS)
#define MCHP_UART_IIR_RO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_IIR_OFS)
#define MCHP_UART_LCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_LCR_OFS)
#define MCHP_UART_MCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_MCR_OFS)
#define MCHP_UART_LSR_RO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_LSR_OFS)
#define MCHP_UART_MSR_RO_ID(id)	\
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_MSR_OFS)
#define MCHP_UART_SCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_SCR_OFS)

/* Register access by UART base address */
#define MCHP_UART_TXB_WO(ba)	REG8_OFS((ba), MCHP_UART_RTXB_OFS)
#define MCHP_UART_RXB_RO(ba)	REG8_OFS((ba), MCHP_UART_RTXB_OFS)
#define MCHP_UART_BRGD_LSB(ba)	REG8_OFS((ba), MCHP_UART_BRGD_LSB_OFS)
#define MCHP_UART_BRGD_MSB(ba)	REG8_OFS((ba), MCHP_UART_BRGD_MSB_OFS)
#define MCHP_UART_FCR_WO(ba)	REG8_OFS((ba), MCHP_UART_FCR_OFS)
#define MCHP_UART_IIR_RO(ba)	REG8_OFS((ba), MCHP_UART_IIR_OFS)
#define MCHP_UART_LCR(ba)	REG8_OFS((ba), MCHP_UART_LCR_OFS)
#define MCHP_UART_MCR(ba)	REG8_OFS((ba), MCHP_UART_MCR_OFS)
#define MCHP_UART_LSR_RO(ba)	REG8_OFS((ba), MCHP_UART_LSR_OFS)
#define MCHP_UART_MSR_RO(ba)	REG8_OFS((ba), MCHP_UART_MSR_OFS)
#define MCHP_UART_SCR(ba)	REG8_OFS((ba), MCHP_UART_SCR_OFS)

#endif /* #ifndef _MEC_MCHP_UART_H */
