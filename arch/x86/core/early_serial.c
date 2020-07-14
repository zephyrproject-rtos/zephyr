/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/util.h>
#include <drivers/pcie/pcie.h>
#include <soc.h>

/* Legacy I/O Port Access to a NS16550 UART */
#ifdef UART_NS16550_ACCESS_IOPORT
#define IN(reg)       sys_in8(reg + UART_NS16550_ACCESS_IOPORT)
#define OUT(reg, val) sys_out8(val, reg + UART_NS16550_ACCESS_IOPORT)
#endif

/* "Modern" mapping of a UART into a PCI MMIO device.  The registers
 * are still bytes, but spaced at a 32 bit stride instead of packed
 * together.
 */
#ifdef X86_SOC_EARLY_SERIAL_PCIDEV
static uintptr_t pci_bar;
#define IN(reg)       (sys_read32(pci_bar + reg * 4) & 0xff)
#define OUT(reg, val) sys_write32((val) & 0xff, pci_bar + reg * 4)
#endif

/* Still other devices use a MMIO region containing packed byte
 * registers
 */
#ifdef X86_SOC_EARLY_SERIAL_MMIO8_ADDR
#define IN(reg)       sys_read8(X86_SOC_EARLY_SERIAL_MMIO8_ADDR + reg)
#define OUT(reg, val) sys_write8(val, X86_SOC_EARLY_SERIAL_MMIO8_ADDR + reg)
#endif

#define REG_THR  0x00  /* Transmitter holding reg. */
#define REG_IER  0x01  /* Interrupt enable reg.    */
#define REG_FCR  0x02  /* FIFO control reg.        */
#define REG_LCR  0x03  /* Line control reg.        */
#define REG_MCR  0x04  /* Modem control reg.       */
#define REG_LSR  0x05  /* Line status reg.         */
#define REG_BRDL 0x00  /* Baud rate divisor (LSB)  */
#define REG_BRDH 0x01  /* Baud rate divisor (MSB)  */

#define IER_DISABLE     0x00
#define LCR_8N1         (BIT(0) | BIT(1))
#define LCR_DLAB_SELECT BIT(7)
#define MCR_DTR         BIT(0)
#define MCR_RTS         BIT(1)
#define LSR_THRE        BIT(5)

#define FCR_FIFO    BIT(0)  /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR BIT(1)  /* clear RCVR FIFO           */
#define FCR_XMITCLR BIT(2)  /* clear XMIT FIFO           */
#define FCR_FIFO_1  0       /* 1 byte in RCVR FIFO       */

static void serout(int c)
{
	while ((IN(REG_LSR) & LSR_THRE) == 0) {
	}
	OUT(REG_THR, c);
}

int arch_printk_char_out(int c)
{
	if (c == '\n') {
		serout('\r');
	}
	serout(c);
	return c;
}

void z_x86_early_serial_init(void)
{
#ifdef X86_SOC_EARLY_SERIAL_PCIDEV
	pci_bar = pcie_get_mbar(X86_SOC_EARLY_SERIAL_PCIDEV, 0);
	pcie_set_cmd(X86_SOC_EARLY_SERIAL_PCIDEV, PCIE_CONF_CMDSTAT_MEM, true);
#endif

	OUT(REG_IER, IER_DISABLE);     /* Disable interrupts */
	OUT(REG_LCR, LCR_DLAB_SELECT); /* DLAB select */
	OUT(REG_BRDL, 1);              /* Baud divisor = 1 */
	OUT(REG_BRDH, 0);
	OUT(REG_LCR, LCR_8N1);         /* LCR = 8n1 + DLAB off */
	OUT(REG_MCR, MCR_DTR | MCR_RTS);

	/* Turn on FIFO. Some hardware needs this before transmitting */
	OUT(REG_FCR, FCR_FIFO | FCR_FIFO_1 | FCR_RCVRCLR | FCR_XMITCLR);
}
