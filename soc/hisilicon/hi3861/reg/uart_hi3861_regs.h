/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_HISILICON_HI3861_REG_UART_HI3861_REGS_H_
#define ZEPHYR_SOC_HISILICON_HI3861_REG_UART_HI3861_REGS_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

struct hi3861_uart_regs {
	volatile uint32_t dr;
	volatile uint32_t rsr_ecr;
	uint32_t reserved0[4];
	volatile const uint32_t fr;
	uint32_t reserved1[2];
	volatile uint32_t ibrd;
	volatile uint32_t fbrd;
	volatile uint32_t lcr_h;
	volatile uint32_t cr;
	volatile uint32_t ifls;
	volatile uint32_t imsc;
	volatile const uint32_t ris;
	volatile const uint32_t mis;
	volatile uint32_t icr;
	volatile uint32_t dmacr;
};

#define UART_DR      offsetof(struct hi3861_uart_regs, dr)
#define UART_RSR_ECR offsetof(struct hi3861_uart_regs, rsr_ecr)
#define UART_FR      offsetof(struct hi3861_uart_regs, fr)
#define UART_IBRD    offsetof(struct hi3861_uart_regs, ibrd)
#define UART_FBRD    offsetof(struct hi3861_uart_regs, fbrd)
#define UART_LCR_H   offsetof(struct hi3861_uart_regs, lcr_h)
#define UART_CR      offsetof(struct hi3861_uart_regs, cr)
#define UART_IFLS    offsetof(struct hi3861_uart_regs, ifls)
#define UART_IMSC    offsetof(struct hi3861_uart_regs, imsc)
#define UART_MIS     offsetof(struct hi3861_uart_regs, mis)
#define UART_ICR     offsetof(struct hi3861_uart_regs, icr)
#define UART_DMACR   offsetof(struct hi3861_uart_regs, dmacr)

#define FR_BUSY BIT(3)
#define FR_RXFE BIT(4)
#define FR_TXFF BIT(5)
#define FR_TXFE BIT(7)

#define RSR_FE BIT(0)
#define RSR_PE BIT(1)
#define RSR_BE BIT(2)
#define RSR_OE BIT(3)

#define LCR_PEN        BIT(1)
#define LCR_EPS        BIT(2)
#define LCR_STP2       BIT(3)
#define LCR_FEN        BIT(4)
#define LCR_WLEN_SHIFT 5
#define LCR_SPS        BIT(7)

#define CR_UARTEN BIT(0)
#define CR_TXE    BIT(8)
#define CR_RXE    BIT(9)
#define CR_RTSEN  BIT(14)
#define CR_CTSEN  BIT(15)

#define INT_RX      BIT(4)
#define INT_TX      BIT(5)
#define INT_RT      BIT(6)
#define INT_ERR     GENMASK(10, 7)
#define INT_RX_MASK (INT_RX | INT_RT)
#define INT_ALL     (INT_RX_MASK | INT_TX | INT_ERR)

#define IFLS_RX_HALF   (2U << 3)
#define IFLS_RTS_HALF  (2U << 6)
#define IFLS_TX_EIGHTH 0U

BUILD_ASSERT(UART_DR == 0x00 && UART_RSR_ECR == 0x04);
BUILD_ASSERT(UART_FR == 0x18);
BUILD_ASSERT(UART_IBRD == 0x24 && UART_FBRD == 0x28);
BUILD_ASSERT(UART_LCR_H == 0x2c && UART_CR == 0x30);
BUILD_ASSERT(UART_IFLS == 0x34 && UART_IMSC == 0x38);
BUILD_ASSERT(offsetof(struct hi3861_uart_regs, ris) == 0x3c && UART_MIS == 0x40);
BUILD_ASSERT(UART_ICR == 0x44 && UART_DMACR == 0x48);
BUILD_ASSERT(sizeof(struct hi3861_uart_regs) == 0x4c);

#endif /* ZEPHYR_SOC_HISILICON_HI3861_REG_UART_HI3861_REGS_H_ */
