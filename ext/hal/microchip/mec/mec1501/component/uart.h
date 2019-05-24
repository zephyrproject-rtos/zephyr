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

/** @file uart.h
 *MEC1501 UART Peripheral Library API
 */
/** @defgroup MEC1501 Peripherals UART
 */

#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

#define MCHP_UART0_ID		0u
#define MCHP_UART1_ID		1u
#define MCHP_UART2_ID		2u
#define MCHP_UART_MAX_ID	3u

#define MCHP_UART_RX_FIFO_MAX_LEN	16u
#define MCHP_UART_TX_FIFO_MAX_LEN	16u

#define MCHP_UART_BAUD_RATE_MIN		50ul
#define MCHP_UART_BAUD_RATE_MAX		1500000ul

#define MCHP_UART0_BASE_ADDRESS		0x400F2400ul
#define MCHP_UART1_BASE_ADDRESS		0x400F2800ul
#define MCHP_UART2_BASE_ADDRESS		0x400F2C00ul

#define MCHP_UART_BASE_ADDR(n) \
	(MCHP_UART0_BASE_ADDRESS + ((uint32_t)(n) << 10u))

#define MCHP_UART_GIRQ_NUM		15u
#define MCHP_UART_GIRQ_ID		7u
#define MCHP_UART_GIRQ_SRC_ADDR		0x4000E08Cul
#define MCHP_UART_GIRQ_EN_SET_ADDR	0x4000E090ul
#define MCHP_UART_GIRQ_RESULT_ADDR	0x4000E094ul
#define MCHP_UART_GIRQ_EN_CLR_ADDR	0x4000E098ul
#define MCHP_UART0_GIRQ_BIT		0u
#define MCHP_UART1_GIRQ_BIT		1u
#define MCHP_UART2_GIRQ_BIT		4u
#define MCHP_UART0_GIRQ_VAL		(1ul << (MCHP_UART0_GIRQ_BIT))
#define MCHP_UART1_GIRQ_VAL		(1ul << (MCHP_UART1_GIRQ_BIT))
#define MCHP_UART2_GIRQ_VAL		(1ul << (MCHP_UART2_GIRQ_BIT))
#define MCHP_UART0_NVIC_DIRECT_NUM	40u
#define MCHP_UART1_NVIC_DIRECT_NUM	41u
#define MCHP_UART2_NVIC_DIRECT_NUM	44u
#define MCHP_UART_NVIC_SETEN_GIRQ_ADDR	0xE000E100ul
#define MCHP_UART_NVIC_SETEN_GIRQ_BIT	7u	/* aggregated */
#define MCHP_UART_NVIC_SETEN_DIRECT_ADDR	0xE000E104ul
#define MCHP_UART0_NVIC_SETEN_DIRECT_BIT	(40u - 32u)
#define MCHP_UART1_NVIC_SETEN_DIRECT_BIT	(41u - 32u)
#define MCHP_UART2_NVIC_SETEN_DIRECT_BIT	(44u - 32u)

/* CMSIS NVIC macro IRQn parameter */
#define MCHP_UART_NVIC_IRQn	7ul	/* aggregated */
#define MCHP_UART0_NVIC_IRQn	41ul	/* UART0 direct mode */
#define MCHP_UART1_NVIC_IRQn	42ul	/* UART1 direct mode */
#define MCHP_UART2_NVIC_IRQn	44ul	/* UART2 direct mode */

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
/* interrupt values */
#define MCHP_UART_IIR_INT_NONE		0x01u
#define MCHP_UART_IIR_INT_LS		0x06u	/* Highest priority: Line status, overrun, framing, or break */
#define MCHP_UART_IIR_INT_RX		0x04u	/* Highest-1. RX data available or RX FIFO trigger level reached */
#define MCHP_UART_IIR_INT_RX_TMOUT	0x0Cu	/* Highest-2. RX timeout */
#define MCHP_UART_IIR_INT_THRE		0x02u	/* Highest-3. TX Holding register empty. */
#define MCHP_UART_IIR_INT_MS		0x00u	/* Highest-4. MODEM status. */

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
#define MCHP_UART_LSR_OFS	5u
#define MCHP_UART_LSR_DATA_RDY	0x01u
#define MCHP_UART_LSR_OVERRUN	0x02u
#define MCHP_UART_LSR_PARITY	0x04u
#define MCHP_UART_LSR_FRAME	0x08u
#define MCHP_UART_LSR_RX_BREAK	0x10u
#define MCHP_UART_LSR_THRE	0x20u
#define MCHP_UART_LSR_TEMT	0x40u
#define MCHP_UART_LSR_FIFO_ERR	0x80u
#define MCHP_UART_LSR_ANY	0xFFu

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
#define MCHP_UART_LD_CFG_INTCLK		(0u << 0)
#define MCHP_UART_LD_CFG_EXTCLK		(1u << 0)
#define MCHP_UART_LD_CFG_RESET_SYS	(0u << 1)
#define MCHP_UART_LD_CFG_RESET_VCC	(1u << 1)
#define MCHP_UART_LD_CFG_NO_INVERT	(0u << 2)
#define MCHP_UART_LD_CFG_INVERT		(1u << 2)

/* BAUD rate generator */
#define MCHP_UART_INT_CLK_24M	(1ul << 15)

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
#define MCHP_UART_TXB_WO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_RTXB_OFS)
#define MCHP_UART_RXB_RO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_RTXB_OFS)
#define MCHP_UART_BRGD_LSB_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_BRGD_LSB_OFS)
#define MCHP_UART_BRGD_MSB_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_BRGD_MSB_OFS)
#define MCHP_UART_FCR_WO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_FCR_OFS)
#define MCHP_UART_IIR_RO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_IIR_OFS)
#define MCHP_UART_LCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_LCR_OFS)
#define MCHP_UART_MCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_MCR_OFS)
#define MCHP_UART_LSR_RO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_LSR_OFS)
#define MCHP_UART_MSR_RO_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_MSR_OFS)
#define MCHP_UART_SCR_ID(id) \
	REG8(MCHP_UART_BASE_ADDR(id) + MCHP_UART_SCR_OFS)

/*
 * Register access by UART base address
 */
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

/* =========================================================================*/
/* ================	       UART			   ================ */
/* =========================================================================*/

/**
  * @brief UART interface (UART)
  */

#define MCHP_UART_NUM_INSTANCES		3u
#define MCHP_UART_SPACING		0x400ul
#define MCHP_UART_SPACING_PWROF2	10u

typedef struct uart_regs
{		/*!< (@ 0x400F2400) UART Structure   */
	__IOM uint8_t RTXB;	/*!< (@ 0x0000) UART RXB(RO), TXB(WO). BRGD_LSB(RW LCR.DLAB=1) */
	__IOM uint8_t IER;	/*!< (@ 0x0001) UART IER(RW). BRGD_MSB(RW LCR.DLAB=1) */
	__IOM uint8_t IIR_FCR;	/*!< (@ 0x0002) UART IIR(RO), FCR(WO) */
	__IOM uint8_t LCR;	/*!< (@ 0x0003) UART Line Control(RW) */
	__IOM uint8_t MCR;	/*!< (@ 0x0004) UART Modem Control(RW) */
	__IOM uint8_t LSR;	/*!< (@ 0x0005) UART Line Status(RO) */
	__IOM uint8_t MSR;	/*!< (@ 0x0006) UART Modem Status(RO) */
	__IOM uint8_t SCR;	/*!< (@ 0x0007) UART Scratch(RW) */
	uint8_t RSVDA[0x330u - 0x08u];
	__IOM uint8_t ACTV;	/*!< (@ 0x0330) UART Activate(RW) */
	uint8_t RSVDB[0x3F0u - 0x331u];
	__IOM uint8_t CFG_SEL;	/*!< (@ 0x03F0) UART Configuration Select(RW) */
} UART_Type;

#endif				/* #ifndef _MCHP_UART_H */
/* end uart.h */
/**   @}
 */
