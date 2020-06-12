/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/util.h>
#include <drivers/pcie/pcie.h>

#include <soc.h>

#if defined(X86_SOC_EARLY_SERIAL_PCIDEV)
#define UART_PCIE_BDF X86_SOC_EARLY_SERIAL_PCIDEV
#define UART_NS16550_PCIE_ENABLED 1
#elif defined(UART_NS16550_ACCESS_IOPORT)
#undef UART_NS16550_PCIE_ENABLED
#else
#error "Incomplete x86 SoC early serial config"
#endif

/* Super-primitive 8250/16550 serial output-only driver, 115200 8n1 */
#define REG_OFFSET_THR		0x00	/* Transmitter holding reg.       */
#define REG_OFFSET_IER		0x01	/* Interrupt enable reg.          */
#define REG_OFFSET_FCR		0x02	/* FIFO control reg.              */
#define REG_OFFSET_LCR		0x03	/* Line control reg.              */
#define REG_OFFSET_MCR		0x04	/* Modem control reg.             */
#define REG_OFFSET_LSR		0x05	/* Line status reg.               */
#define REG_OFFSET_BRDL		0x00	/* Baud rate divisor (LSB)        */
#define REG_OFFSET_BRDH		0x01	/* Baud rate divisor (MSB)        */

#define IER_DISABLE		0x00
#define LCR_8N1			(BIT(0) | BIT(1))
#define LCR_DLAB_SELECT		BIT(7)
#define MCR_DTR			BIT(0)
#define MCR_RTS			BIT(1)
#define LSR_THRE		BIT(5)

#define FCR_FIFO		BIT(0)	/* enable XMIT and RCVR FIFO      */
#define FCR_RCVRCLR		BIT(1)	/* clear RCVR FIFO                */
#define FCR_XMITCLR		BIT(2)	/* clear XMIT FIFO                */
#define FCR_FIFO_1		0	/* 1 byte in RCVR FIFO            */

/* convenience defines */
#define REG_THR(x)		(x + REG_OFFSET_THR * UART_REG_ADDR_INTERVAL)
#define REG_IER(x)		(x + REG_OFFSET_IER * UART_REG_ADDR_INTERVAL)
#define REG_FCR(x)		(x + REG_OFFSET_FCR * UART_REG_ADDR_INTERVAL)
#define REG_LCR(x)		(x + REG_OFFSET_LCR * UART_REG_ADDR_INTERVAL)
#define REG_MCR(x)		(x + REG_OFFSET_MCR * UART_REG_ADDR_INTERVAL)
#define REG_LSR(x)		(x + REG_OFFSET_LSR * UART_REG_ADDR_INTERVAL)
#define REG_BRDL(x)		(x + REG_OFFSET_BRDL * UART_REG_ADDR_INTERVAL)
#define REG_BRDH(x)		(x + REG_OFFSET_BRDH * UART_REG_ADDR_INTERVAL)

#ifdef UART_NS16550_ACCESS_IOPORT
#define INBYTE(x)	sys_in8(x)
#define OUTBYTE(x, d)	sys_out8(d, x)
#ifndef UART_REG_ADDR_INTERVAL
#define UART_REG_ADDR_INTERVAL 1 /* address diff of adjacent regs. */
#endif /* UART_REG_ADDR_INTERVAL */
#else
#define INBYTE(x)	sys_read8(x)
#define OUTBYTE(x, d)	sys_write8(d, x)
#ifndef UART_REG_ADDR_INTERVAL
#define UART_REG_ADDR_INTERVAL 4 /* address diff of adjacent regs. */
#endif
#endif /* UART_NS16550_ACCESS_IOPORT */

#ifdef UART_NS16550_PCIE_ENABLED
static mm_reg_t base;
#else
#define base PORT
#endif

static void serout(int c)
{
	while ((INBYTE(REG_LSR(base)) & LSR_THRE) == 0) {
	}
	OUTBYTE(REG_THR(base), c);
}

static int console_out(int c)
{
	if (c == '\n') {
		serout('\r');
	}
	serout(c);
	return c;
}

extern void __printk_hook_install(int (*fn)(int));

void z_x86_early_serial_init(void)
{
#ifdef UART_NS16550_PCIE_ENABLED
	base = pcie_get_mbar(UART_PCIE_BDF, 0);
	pcie_set_cmd(UART_PCIE_BDF, PCIE_CONF_CMDSTAT_MEM, true);
#endif

	OUTBYTE(REG_IER(base), IER_DISABLE);	/* Disable interrupts */
	OUTBYTE(REG_LCR(base), LCR_DLAB_SELECT);/* DLAB select */
	OUTBYTE(REG_BRDL(base), 1);		/* Baud divisor = 1 */
	OUTBYTE(REG_BRDH(base), 0);
	OUTBYTE(REG_LCR(base), LCR_8N1);	/* LCR = 8n1 + DLAB off */
	OUTBYTE(REG_MCR(base), MCR_DTR | MCR_RTS);

	/* Turn on FIFO. Some hardware needs this before transmitting */
	OUTBYTE(REG_FCR(base),
		FCR_FIFO | FCR_FIFO_1 | FCR_RCVRCLR | FCR_XMITCLR);

	/* Will be replaced later when a real serial driver comes up */
	__printk_hook_install(console_out);
}
