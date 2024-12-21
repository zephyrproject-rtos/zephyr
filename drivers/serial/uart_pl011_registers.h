/*
 * Copyright (c) 2018 Linaro Limited
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_PL011_REGISTERS_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_PL011_REGISTERS_H_

#include <zephyr/device.h>

/*
 * UART PL011 register map structure
 */
struct pl011_regs {
	uint32_t dr;			/* data register */
	union {
		uint32_t rsr;
		uint32_t ecr;
	};
	uint32_t reserved_0[4];
	uint32_t fr;			/* flags register */
	uint32_t reserved_1;
	uint32_t ilpr;
	uint32_t ibrd;
	uint32_t fbrd;
	uint32_t lcr_h;
	uint32_t cr;
	uint32_t ifls;
	uint32_t imsc;
	uint32_t ris;
	uint32_t mis;
	uint32_t icr;
	uint32_t dmacr;
};

static inline
volatile struct pl011_regs *get_uart(const struct device *dev)
{
	return (volatile struct pl011_regs *)DEVICE_MMIO_GET(dev);
}

#define PL011_BIT_MASK(x, y) (((2 << x) - 1) << y)

/* PL011 Uart Flags Register */
#define PL011_FR_CTS		BIT(0)	/* clear to send - inverted */
#define PL011_FR_DSR		BIT(1)	/* data set ready - inverted */
#define PL011_FR_DCD		BIT(2)	/* data carrier detect - inverted */
#define PL011_FR_BUSY		BIT(3)	/* busy transmitting data */
#define PL011_FR_RXFE		BIT(4)	/* receive FIFO empty */
#define PL011_FR_TXFF		BIT(5)	/* transmit FIFO full */
#define PL011_FR_RXFF		BIT(6)	/* receive FIFO full */
#define PL011_FR_TXFE		BIT(7)	/* transmit FIFO empty */
#define PL011_FR_RI		BIT(8)	/* ring indicator - inverted */

/* PL011 Integer baud rate register */
#define PL011_IBRD_BAUD_DIVINT_MASK	0xff	/* 16 bits of divider */

/* PL011 Fractional baud rate register */
#define PL011_FBRD_BAUD_DIVFRAC		0x3f
#define PL011_FBRD_WIDTH		6u

/* PL011 Receive status register / error clear register */
#define PL011_RSR_ECR_FE	BIT(0)	/* framing error */
#define PL011_RSR_ECR_PE	BIT(1)	/* parity error */
#define PL011_RSR_ECR_BE	BIT(2)	/* break error */
#define PL011_RSR_ECR_OE	BIT(3)	/* overrun error */

#define PL011_RSR_ERROR_MASK	(PL011_RSR_ECR_FE | PL011_RSR_ECR_PE | \
		PL011_RSR_ECR_BE | PL011_RSR_ECR_OE)

/* PL011 Line Control Register  */
#define PL011_LCRH_BRK		BIT(0)	/* send break */
#define PL011_LCRH_PEN		BIT(1)	/* enable parity */
#define PL011_LCRH_EPS		BIT(2)	/* select even parity */
#define PL011_LCRH_STP2		BIT(3)	/* select two stop bits */
#define PL011_LCRH_FEN		BIT(4)	/* enable FIFOs */
#define PL011_LCRH_WLEN_SHIFT	5	/* word length */
#define PL011_LCRH_WLEN_WIDTH	2
#define PL011_LCRH_SPS		BIT(7)	/* stick parity bit */

#define PL011_LCRH_WLEN_SIZE(x) (x - 5)

#define PL011_LCRH_FORMAT_MASK	(PL011_LCRH_PEN | PL011_LCRH_EPS | \
		PL011_LCRH_SPS | \
		PL011_BIT_MASK(PL011_LCRH_WLEN_WIDTH, PL011_LCRH_WLEN_SHIFT))

#define PL011_LCRH_PARTIY_EVEN	(PL011_LCRH_PEN | PL011_LCRH_EPS)
#define PL011_LCRH_PARITY_ODD	(PL011_LCRH_PEN)
#define PL011_LCRH_PARITY_NONE	(0)

/* PL011 Control Register */
#define PL011_CR_UARTEN		BIT(0)	/* enable uart operations */
#define PL011_CR_SIREN		BIT(1)	/* enable IrDA SIR */
#define PL011_CR_SIRLP		BIT(2)	/* IrDA SIR low power mode */
#define PL011_CR_LBE		BIT(7)	/* loop back enable */
#define PL011_CR_TXE		BIT(8)	/* transmit enable */
#define PL011_CR_RXE		BIT(9)	/* receive enable */
#define PL011_CR_DTR		BIT(10)	/* data transmit ready */
#define PL011_CR_RTS		BIT(11)	/* request to send */
#define PL011_CR_Out1		BIT(12)
#define PL011_CR_Out2		BIT(13)
#define PL011_CR_RTSEn		BIT(14)	/* RTS hw flow control enable */
#define PL011_CR_CTSEn		BIT(15)	/* CTS hw flow control enable */

/* PL011 Control Register - vendor-specific fields */
#define PL011_CR_AMBIQ_CLKEN	BIT(3)	/* clock enable */
#define PL011_CR_AMBIQ_CLKSEL	GENMASK(6, 4) /* clock select */
#define PL011_CR_AMBIQ_CLKSEL_NOCLK	0
#define PL011_CR_AMBIQ_CLKSEL_24MHZ	1
#define PL011_CR_AMBIQ_CLKSEL_12MHZ	2
#define PL011_CR_AMBIQ_CLKSEL_6MHZ	3
#define PL011_CR_AMBIQ_CLKSEL_3MHZ	4
#define PL011_CR_AMBIQ_CLKSEL_48MHZ	5

/* PL011 Interrupt Fifo Level Select Register */
#define PL011_IFLS_RXIFLSEL_M	GENMASK(5, 3)
#define RXIFLSEL_1_2_FULL	2UL
#define PL011_IFLS_TXIFLSEL_M	GENMASK(2, 0)
#define TXIFLSEL_1_8_FULL	0UL

/* PL011 Interrupt Mask Set/Clear Register */
#define PL011_IMSC_RIMIM	BIT(0)	/* RTR modem interrupt mask */
#define PL011_IMSC_CTSMIM	BIT(1)	/* CTS modem interrupt mask */
#define PL011_IMSC_DCDMIM	BIT(2)	/* DCD modem interrupt mask */
#define PL011_IMSC_DSRMIM	BIT(3)	/* DSR modem interrupt mask */
#define PL011_IMSC_RXIM		BIT(4)	/* receive interrupt mask */
#define PL011_IMSC_TXIM		BIT(5)	/* transmit interrupt mask */
#define PL011_IMSC_RTIM		BIT(6)	/* receive timeout interrupt mask */
#define PL011_IMSC_FEIM		BIT(7)	/* framing error interrupt mask */
#define PL011_IMSC_PEIM		BIT(8)	/* parity error interrupt mask */
#define PL011_IMSC_BEIM		BIT(9)	/* break error interrupt mask */
#define PL011_IMSC_OEIM		BIT(10)	/* overrun error interrupt mask */

#define PL011_IMSC_ERROR_MASK	(PL011_IMSC_FEIM | \
		PL011_IMSC_PEIM | PL011_IMSC_BEIM | \
		PL011_IMSC_OEIM)

#define PL011_IMSC_MASK_ALL (PL011_IMSC_OEIM | PL011_IMSC_BEIM | \
		PL011_IMSC_PEIM | PL011_IMSC_FEIM | \
		PL011_IMSC_RIMIM | PL011_IMSC_CTSMIM | \
		PL011_IMSC_DCDMIM | PL011_IMSC_DSRMIM | \
		PL011_IMSC_RXIM | PL011_IMSC_TXIM | \
		PL011_IMSC_RTIM)

/* PL011 Raw Interrupt Status Register */
#define PL011_RIS_TXRIS		BIT(5)	/* Transmit interrupt status */


#endif /* ZEPHYR_DRIVERS_SERIAL_UART_PL011_REGISTERS_H_ */
